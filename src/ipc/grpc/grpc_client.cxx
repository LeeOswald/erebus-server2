#include <erebus/erebus.grpc.pb.h>
#include <grpcpp/grpcpp.h>

#include "protocol.hxx"
#include "trace.hxx"

#include <erebus/ipc/grpc/grpc_client.hxx>
#include <erebus/system/system/packed_time.hxx>
#include <erebus/system/property_info.hxx>
#include <erebus/system/util/exception_util.hxx>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <vector>

#include <boost/noncopyable.hpp>


namespace Er::Ipc::Grpc
{

namespace
{

static Er::ResultCode mapGrpcStatus(grpc::StatusCode status) noexcept
{
    switch (status)
    {
    case grpc::OK: return Er::Result::Ok;
    case grpc::CANCELLED: return Result::Canceled;
    case grpc::UNKNOWN: return Result::Failure;
    case grpc::INVALID_ARGUMENT: return Result::InvalidArgument;
    case grpc::DEADLINE_EXCEEDED: return Result::Timeout;
    case grpc::NOT_FOUND: return Result::NotFound;
    case grpc::ALREADY_EXISTS: return Result::AlreadyExists;
    case grpc::PERMISSION_DENIED: return Result::AccessDenied;
    case grpc::UNAUTHENTICATED: return Result::Unauthenticated;
    case grpc::RESOURCE_EXHAUSTED: return Result::ResourceExhausted;
    case grpc::FAILED_PRECONDITION: return Result::FailedPrecondition;
    case grpc::ABORTED: return Result::Aborted;
    case grpc::OUT_OF_RANGE: return Result::OutOfRange;
    case grpc::UNIMPLEMENTED: return Result::Unimplemented;
    case grpc::INTERNAL: return Result::Internal;
    case grpc::UNAVAILABLE: return Result::Unavailable;
    case grpc::DATA_LOSS: return Result::DataLoss;
    default: return Result::Failure;
    }
}


class ClientImpl final
    : public IClient
    , public Er::IPropertyMapping
    , public boost::noncopyable
{
public:
    ~ClientImpl()
    {
        ClientTrace2(m_log, "{}.ClientImpl::~ClientImpl()", Er::Format::ptr(this));

        waitRunningContexts();

        ::grpc_shutdown();
    }

    explicit ClientImpl(std::shared_ptr<grpc::Channel> channel, Er::Log2::ILogger::Ptr log)
        : m_grpcReady(grpcInit())
        , m_stub(erebus::Erebus::NewStub(channel))
        , m_logRef(log)
        , m_log(log.get())
        , m_clientId(makeClientId())
    {
        ClientTrace2(m_log, "{}.ClientImpl::ClientImpl()", Er::Format::ptr(this));
    }

private:
    struct ContextBase
        : public boost::noncopyable
    {
        ~ContextBase() noexcept
        {
            ClientTrace2(m_log, "{}.ContextBase::~ContextBase()", Er::Format::ptr(this));
            m_owner->removeContext();
        }

        ContextBase(ClientImpl* owner, Er::Log2::ILogger* log) noexcept
            : m_owner(owner)
            , m_log(log)
        {
            ClientTrace2(m_log, "{}.ContextBase::ContextBase()", Er::Format::ptr(this));
            owner->addContext();
        }

    protected:
        ClientImpl* const m_owner;
        Er::Log2::ILogger* const m_log;
    };

    struct PingContext final
        : public ContextBase
    {
        ~PingContext()
        {
            ClientTrace2(m_log, "{}.PingContext::~PingContext()", Er::Format::ptr(this));
        }

        PingContext(ClientImpl* owner, Er::Log2::ILogger* log, std::uint32_t clientId, std::size_t payloadSize, IClient::IPingCompletion::Ptr handler)
            : ContextBase(owner, log)
            , handler(handler)
            , started(Er::System::PackedTime::now())
        {
            ClientTrace2(m_log, "{}.PingContext::PingContext(clientId={})", Er::Format::ptr(this), clientId);

            request.set_clientid(clientId);
            request.set_timestamp(started);
            if (payloadSize)
            {
                auto payload = makeNoise(payloadSize);
                request.set_payload(std::move(payload));
            }
        }

        IClient::IPingCompletion::Ptr handler;
        Er::System::PackedTime::ValueType started;
        erebus::PingRequest request;
        grpc::ClientContext context;
        erebus::PingReply reply;
    };

    struct CallContext final
        : public ContextBase
    {
        ~CallContext()
        {
            ClientTrace2(m_log, "{}.CallContext::~CallContext()", Er::Format::ptr(this));
        }

        CallContext(ClientImpl* owner, Er::Log2::ILogger* log, std::string_view req, const Er::PropertyBag& args, std::uint32_t clientId, IClient::ICallCompletion::Ptr handler)
            : ContextBase(owner, log)
            , uri(req)
            , handler(handler)
        {
            ClientTrace2(m_log, "{}.CallContext::CallContext(req={} clientId={})", Er::Format::ptr(this), req, clientId);

            request.set_request(std::string(req));
            request.set_clientid(clientId);
            request.set_mappingver(Erp::propertyMappingVersion());

            for (auto& arg: args)
            {
                auto a = request.add_args();
                Erp::Protocol::assignProperty(*a, arg);
            }
        }

        std::string uri;
        IClient::ICallCompletion::Ptr handler;
        erebus::ServiceRequest request;
        grpc::ClientContext context;
        erebus::ServiceReply reply;
    };
    
    struct ServiceReplyStreamReader final
        : public grpc::ClientReadReactor<erebus::ServiceReply>
        , public ContextBase
    {
        ~ServiceReplyStreamReader()
        {
            ClientTrace2(m_log, "{}.ServiceReplyStreamReader::~ServiceReplyStreamReader()", Er::Format::ptr(this));
        }

        ServiceReplyStreamReader(
            ClientImpl* owner, 
            Er::Log2::ILogger* log, 
            erebus::Erebus::Stub* stub, 
            std::string_view req, 
            const Er::PropertyBag& args, 
            std::uint32_t clientId, 
            IStreamCompletion::Ptr handler
        )
            : ContextBase(owner, log)
            , m_uri(req)
            , m_handler(handler)
        {
            ClientTraceIndent2(m_log, "{}.ServiceReplyStreamReader::ServiceReplyStreamReader(clientId={})", Er::Format::ptr(this), m_uri, clientId);

            m_request.set_request(std::string(req));
            m_request.set_clientid(clientId);
            auto ver = Erp::propertyMappingVersion();
            m_request.set_mappingver(ver);

            ClientTrace2(m_log, "Sending property mapping v.{}", ver);

            for (auto& arg : args)
            {
                auto a = m_request.add_args();
                Erp::Protocol::assignProperty(*a, arg);
            }

            stub->async()->GenericStream(&m_context, &m_request, this);
            StartRead(&m_reply);
            StartCall();
        }

        void OnReadDone(bool ok) override
        {
            ClientTraceIndent2(m_log, "{}.ServiceReplyStreamReader::OnReadDone({}, {})", Er::Format::ptr(this), m_uri, ok);

            if (!ok)
                return;

            Er::Util::ExceptionLogger xcptLogger(m_log);

            try
            {
                if (m_reply.result() != erebus::CallResult::SUCCESS)
                {
                    auto code = m_reply.result();
                    if (code == erebus::CallResult::PROPERTY_MAPPING_EXPIRED)
                    {
                        ClientTrace2(m_log, "Server property mapping expired for {}:{}", m_context.peer(), m_uri);
                        m_handler->onServerPropertyMappingExpired();
                        
                        m_context.TryCancel();
                        return StartRead(&m_reply);
                    }
                    else if (!m_reply.has_exception())
                    {
                        auto message = Er::format("Unexpected error streaming from {}:{}: {}", m_context.peer(), m_uri, static_cast<int>(code));
                        m_handler->onTransportError(Er::Result::Failure, std::move(message));

                        m_context.TryCancel();
                        return StartRead(&m_reply);
                    }
                }

                auto remoteMappingVer = m_reply.mappingver();
                auto localMappingVer = m_owner-> m_propertyMapping.version;
                if (remoteMappingVer != localMappingVer)
                {
                    ClientTrace2(m_log, "Client property mapping expired for {}:{} (remote v.{} local v.{})", m_context.peer(), m_uri, remoteMappingVer, localMappingVer);
                    m_handler->onClientPropertyMappingExpired();

                    m_context.TryCancel();
                    return StartRead(&m_reply);
                }

                if (m_reply.has_exception())
                {
                    auto e = m_owner->unmarshalException(m_reply);
                    ErLogError2(m_log, "Exception while streaming from {}:{}: {}", m_context.peer(), m_uri, e.what());

                    m_handler->onException(std::move(e));
                    
                    m_context.TryCancel();
                    return StartRead(&m_reply);
                }
                
                auto reply = m_owner->unmarshal(m_reply);
                if (m_handler->onFrame(std::move(reply)) == Er::CallbackResult::Cancel)
                {
                    m_context.TryCancel();
                }
                
            }
            catch (...)
            {
                Er::dispatchException(std::current_exception(), xcptLogger);
            }

            // we have to drain the completion queue even if we cancel
            StartRead(&m_reply);
        }

        void OnDone(const grpc::Status& status) override
        {
            {
                ClientTraceIndent2(m_log, "{}.ServiceReplyStreamReader::OnDone({}, {})", Er::Format::ptr(this), m_uri, int(status.error_code()));

                Er::Util::ExceptionLogger xcptLogger(m_log);

                try
                {
                    if (!status.ok())
                    {
                        auto resultCode = mapGrpcStatus(status.error_code());
                        auto errorMsg = status.error_message();
                        ErLogError2(m_log, "Stream from {} terminated with an error: {} ({})", m_context.peer(), resultCode, errorMsg);

                        m_handler->onTransportError(resultCode, std::move(errorMsg));
                    }

                    m_handler->done();
                }
                catch (...)
                {
                    Er::dispatchException(std::current_exception(), xcptLogger);
                }
            }

            delete this;
        }

    private:
        std::string m_uri;
        IStreamCompletion::Ptr m_handler;
        erebus::ServiceRequest m_request;
        grpc::ClientContext m_context;
        erebus::ServiceReply m_reply;
    };

    struct PropertyMappingStreamReader final
        : public grpc::ClientReadReactor<erebus::GetPropertyMappingReply>
        , public ContextBase
    {
        ~PropertyMappingStreamReader()
        {
            ClientTrace2(m_log, "{}.PropertyMappingStreamReader::~PropertyMappingStreamReader()", Er::Format::ptr(this));
        }

        PropertyMappingStreamReader(ClientImpl* owner, Er::Log2::ILogger* log, erebus::Erebus::Stub* stub, std::uint32_t clientId, ICompletion::Ptr handler)
            : ContextBase(owner, log)
            , m_clientId(clientId)
            , m_handler(handler)
        {
            ClientTraceIndent2(m_log, "{}.PropertyMappingStreamReader::PropertyMappingStreamReader(clientId={})", Er::Format::ptr(this), clientId);

            stub->async()->GetPropertyMapping(&m_context, &m_request, this);
            StartRead(&m_reply);
            StartCall();
        }

        void OnReadDone(bool ok) override
        {
            ClientTraceIndent2(m_log, "{}.PropertyMappingStreamReader::OnReadDone({})", Er::Format::ptr(this), ok);

            if (ok)
            {
                Er::Util::ExceptionLogger xcptLogger(m_log);

                try
                {
                    auto version = m_reply.mappingver();

                    auto id = m_reply.mapping().id();
                    auto type = static_cast<Er::PropertyType>(m_reply.mapping().type());
                    auto& name = m_reply.mapping().name();
                    auto& readableName = m_reply.mapping().readablename();

                    m_owner->putPropertyMapping(version, id, type, name, readableName);
                }
                catch (...)
                {
                    Er::dispatchException(std::current_exception(), xcptLogger);
                }

                // we have to drain the completion queue even if we cancel
                StartRead(&m_reply);
            }
        }

        void OnDone(const grpc::Status& status) override
        {
            {
                ClientTraceIndent2(m_log, "{}.PropertyMappingStreamReader::OnDone({})", Er::Format::ptr(this), int(status.error_code()));

                Er::Util::ExceptionLogger xcptLogger(m_log);

                try
                {
                    if (!status.ok())
                    {
                        auto resultCode = mapGrpcStatus(status.error_code());
                        auto errorMsg = status.error_message();
                        ErLogError2(m_log, "Stream from {} terminated with an error: {} ({})", m_context.peer(), resultCode, errorMsg);

                        m_handler->onTransportError(resultCode, std::move(errorMsg));
                    }

                    m_handler->done();
                }
                catch (...)
                {
                    Er::dispatchException(std::current_exception(), xcptLogger);
                }
            }

            delete this;
        }

    private:
        std::uint32_t m_clientId;
        ICompletion::Ptr m_handler;
        erebus::Void m_request;
        grpc::ClientContext m_context;
        erebus::GetPropertyMappingReply m_reply;
    };

    struct PropertyMappingStreamWriter final
        : public grpc::ClientWriteReactor<erebus::PutPropertyMappingRequest>
        , public ContextBase
    {
        ~PropertyMappingStreamWriter()
        {
            ClientTrace2(m_log, "{}.PropertyMappingStreamWriter::~PropertyMappingStreamWriter()", Er::Format::ptr(this));
        }

        PropertyMappingStreamWriter(ClientImpl* owner, Er::Log2::ILogger* log, erebus::Erebus::Stub* stub, std::uint32_t clientId, ICompletion::Ptr handler)
            : ContextBase(owner, log)
            , m_clientId(clientId)
            , m_handler(handler)
        {
            ClientTraceIndent2(m_log, "{}.PropertyMappingStreamWriter::PropertyMappingStreamWriter(clientId={})", Er::Format::ptr(this), clientId);

            m_mappingVer = Er::enumerateProperties(
                [this](const Er::PropertyInfo* pi) -> bool
                {
                    m_mapping.push_back(pi);
                    return true;
                });

            ClientTrace2(m_log, "Sending local property mapping v.{}", m_mappingVer);

            stub->async()->PutPropertyMapping(&m_context, &m_reply, this);

            nextWrite();
            StartCall();
        }

        void OnWriteDone(bool ok) override 
        {
            ClientTraceIndent2(m_log, "{}.PropertyMappingStreamWriter::OnWriteDone({})", Er::Format::ptr(this), ok);

            if (ok) 
            { 
                nextWrite(); 
            }
        }

        void OnDone(const grpc::Status& status) override 
        {
            {
                ClientTraceIndent2(m_log, "{}.PropertyMappingStreamWriter::OnDone({})", Er::Format::ptr(this), static_cast<int>(status.error_code()));

                Er::Util::ExceptionLogger xcptLogger(m_log);

                try
                {
                    if (!status.ok())
                    {
                        auto resultCode = mapGrpcStatus(status.error_code());
                        auto errorMsg = status.error_message();
                        ErLogError2(m_log, "Stream to {} terminated with an error: {} ({})", m_context.peer(), resultCode, errorMsg);

                        m_handler->onTransportError(resultCode, std::move(errorMsg));
                    }

                    m_handler->done();
                }
                catch (...)
                {
                    Er::dispatchException(std::current_exception(), xcptLogger);
                }
            }

            delete this;
        }

    private:
        void nextWrite()
        {
            if (m_nextIndex < m_mapping.size())
            {
                ClientTraceIndent2(m_log, "{}.PropertyMappingStreamWriter::nextWrite(#{} of {})", Er::Format::ptr(this), m_nextIndex, m_mapping.size());

                m_request.set_clientid(m_clientId);
                m_request.set_mappingver(m_mappingVer);

                auto prop = m_mapping[m_nextIndex++];

                auto m = m_request.mutable_mapping();
                m->set_id(prop->unique());
                m->set_type(static_cast<std::uint32_t>(prop->type()));
                m->set_name(prop->name());
                m->set_readablename(prop->readableName());

                StartWrite(&m_request);
            }
            else
            {
                ClientTraceIndent2(m_log, "{}.PropertyMappingStreamWriter::nextWrite(): end of stream", Er::Format::ptr(this));

                StartWritesDone();
            }
        }

    private:
        std::uint32_t m_clientId;
        ICompletion::Ptr m_handler;
        erebus::PutPropertyMappingRequest m_request;
        grpc::ClientContext m_context;
        erebus::Void m_reply;
        std::vector<const Er::PropertyInfo*> m_mapping;
        std::uint32_t m_mappingVer = std::uint32_t(-1);
        std::size_t m_nextIndex = 0;
    };

    const Er::PropertyInfo* mapProperty(std::uint32_t id, std::uint32_t clientId) override
    {
        {
            std::shared_lock l(m_propertyMapping.lock);

            if (id < m_propertyMapping.map.size())
                return m_propertyMapping.map[id];
        }

        ErLogError2(m_log, "{}.ClientImpl::mapProperty({}) -> NULL", Er::Format::ptr(this), id);
        return nullptr;
    }

    void ping(std::size_t payloadSize, IPingCompletion::Ptr handler, std::chrono::milliseconds timeout) override
    {
        ClientTraceIndent2(m_log, "{}.ClientImpl::ping()", Er::Format::ptr(this));

        auto ctx = std::make_shared<PingContext>(this, m_log, m_clientId, payloadSize, handler);
        ctx->context.set_deadline(std::chrono::system_clock::now() + timeout);

        m_stub->async()->Ping(
            &ctx->context,
            &ctx->request,
            &ctx->reply,
            [this, ctx, payloadSize](grpc::Status status)
            {
                completePing(ctx, status, payloadSize);
            });
    }

    void getPropertyMapping(ICompletion::Ptr handler) override
    {
        ClientTraceIndent2(m_log, "{}.ClientImpl::getPropertyMapping()", Er::Format::ptr(this));

        new PropertyMappingStreamReader(this, m_log, m_stub.get(), m_clientId, handler);
    }

    void putPropertyMapping(ICompletion::Ptr handler) override
    {
        ClientTraceIndent2(m_log, "{}.ClientImpl::putPropertyMapping()", Er::Format::ptr(this));

        new PropertyMappingStreamWriter(this, m_log, m_stub.get(), m_clientId, handler);
    }

    void call(std::string_view request, const Er::PropertyBag& args, ICallCompletion::Ptr handler, std::chrono::milliseconds timeout) override
    {
        ClientTraceIndent2(m_log, "{}.ClientImpl::call({})", Er::Format::ptr(this), request);

        auto ctx = std::make_shared<CallContext>(this, m_log, request, args, m_clientId, handler);
        ctx->context.set_deadline(std::chrono::system_clock::now() + timeout);

        m_stub->async()->GenericCall(
            &ctx->context,
            &ctx->request,
            &ctx->reply,
            [this, ctx](grpc::Status status)
            {
                completeCall(ctx, status);
            });
    }

    void stream(std::string_view request, const Er::PropertyBag& args, IStreamCompletion::Ptr handler) override
    {
        ClientTraceIndent2(m_log, "{}.ClientImpl::stream({})", Er::Format::ptr(this), request);

        new ServiceReplyStreamReader(this, m_log, m_stub.get(), request, args, m_clientId, handler);
    }

    void completePing(std::shared_ptr<PingContext> ctx, grpc::Status status, std::size_t payloadSize)
    {
        ClientTraceIndent2(m_log, "{}.ClientImpl::completePing({})", Er::Format::ptr(this), static_cast<int>(status.error_code()));

        Er::Util::ExceptionLogger xcptLogger(m_log);

        try
        {
            if (!status.ok())
            {
                auto resultCode = mapGrpcStatus(status.error_code());
                auto errorMsg = status.error_message();
                ErLogError2(m_log, "Failed to ping {}: {} ({})", ctx->context.peer(), static_cast<int>(status.error_code()), errorMsg);

                ctx->handler->onTransportError(resultCode, std::move(errorMsg));
            }
            else
            {
                auto finished = Er::System::PackedTime::now();
                auto milliseconds = (finished - ctx->started) / 1000;

                ctx->handler->onReply(payloadSize, std::chrono::milliseconds(milliseconds));
            }

            ctx->handler->done();
        }
        catch (...)
        {
            Er::dispatchException(std::current_exception(), xcptLogger);
        }
    }

    void completeCall(std::shared_ptr<CallContext> ctx, grpc::Status status)
    {
        ClientTraceIndent2(m_log, "{}.ClientImpl::completeCall({})", Er::Format::ptr(this), static_cast<int>(status.error_code()));

        Er::Util::ExceptionLogger xcptLogger(m_log);

        try
        {
            if (!status.ok())
            {
                auto resultCode = mapGrpcStatus(status.error_code());
                auto errorMsg = status.error_message();
                ErLogError2(m_log, "Failed to call {}:{}: {} ({})", ctx->context.peer(), ctx->uri, resultCode, errorMsg);

                ctx->handler->onTransportError(resultCode, std::move(errorMsg));

                return ctx->handler->done();
            }

            if (ctx->reply.result() != erebus::CallResult::SUCCESS)
            {
                auto code = ctx->reply.result();
                if (code == erebus::CallResult::PROPERTY_MAPPING_EXPIRED)
                {
                    ClientTrace2(m_log, "Server property mapping expired for {}:{}", ctx->context.peer(), ctx->uri);
                    ctx->handler->onServerPropertyMappingExpired();
                    return ctx->handler->done();
                }
                else if (!ctx->reply.has_exception())
                {
                    auto message = Er::format("Unexpected error calling {}:{}: {}", ctx->context.peer(), ctx->uri, static_cast<int>(code));
                    ctx->handler->onTransportError(Er::Result::Failure, std::move(message));
                    return ctx->handler->done();
                }
            }

            auto remoteMappingVer = ctx->reply.mappingver();
            auto localMappingVer = m_propertyMapping.version;
            if (remoteMappingVer != localMappingVer)
            {
                ClientTrace2(m_log, "Client property mapping expired for {}:{} (remote v.{} local v.{})", ctx->context.peer(), ctx->uri, remoteMappingVer, localMappingVer);
                ctx->handler->onClientPropertyMappingExpired();
                return ctx->handler->done();
            }

            if (ctx->reply.has_exception())
            {
                auto e = unmarshalException(ctx->reply);
                ErLogError2(m_log, "Failed to call {}:{}: {}", ctx->context.peer(), ctx->uri, e.what());

                ctx->handler->onException(std::move(e));
                return ctx->handler->done();
            }
                        
            auto reply = unmarshal(ctx->reply);
            ctx->handler->onReply(std::move(reply));
            ctx->handler->done();
        }
        catch (...)
        {
            Er::dispatchException(std::current_exception(), xcptLogger);
        }
    }

    Er::Exception unmarshalException(const erebus::ServiceReply& reply)
    {
        ErAssert(reply.has_exception());

        auto& exception = reply.exception();
        std::string_view message;
        if (exception.has_message())
            message = exception.message();
        else
            message = "Unknown exception";

        Er::Exception unmarshaledException(std::source_location::current(), std::move(message));

        auto propCount = exception.props_size();

        for (int i = 0; i < propCount; ++i)
        {
            auto& prop = exception.props(i);
            unmarshaledException.add(Erp::Protocol::getProperty(prop, this, m_clientId));
        }

        return unmarshaledException;
    }

    Er::PropertyBag unmarshal(const erebus::ServiceReply& reply)
    {
        Er::PropertyBag bag;
        int count = reply.props_size();
        for (int i = 0; i < count; ++i)
        {
            auto& prop = reply.props(i);

            bag.push_back(Erp::Protocol::getProperty(prop, this, m_clientId));
        }

        return bag;
    }

    void putPropertyMapping(std::uint32_t version, std::uint32_t id, Er::PropertyType type, const std::string& name, const std::string& readableName)
    {
        ClientTraceIndent2(m_log, "{}.ClientImpl::putPropertyMapping(v.{} {} -> {} [{}])", Er::Format::ptr(this), version, id, name, readableName);

        auto pi = Erp::allocateTransientProperty(type, name, readableName);

        std::lock_guard l(m_propertyMapping.lock);

        auto& m = m_propertyMapping.map;
        if (m.size() <= id)
            m.resize(id + 1);

        m[id] = pi;
        m_propertyMapping.version = version;
    }

    static std::string makeNoise(std::size_t length)
    {
        static const char ValidChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
        static const size_t ValidCharsCount = sizeof(ValidChars) - 1;
        
        std::uniform_int_distribution<> charDistrib(0, ValidCharsCount - 1);

        static std::random_device rd;
        std::mt19937 random(rd());

        std::string cookie(length, ' ');
        for (size_t i = 0; i < length; ++i)
            cookie[i] = ValidChars[charDistrib(random)];

        return cookie;
    }

    static std::uint32_t makeClientId() noexcept
    {
        static std::atomic<std::uint32_t> nextId = 0;
        return nextId.fetch_add(1, std::memory_order_relaxed);
    }

    static bool grpcInit() noexcept
    {
        ::grpc_init();
        return true;
    }

    void addContext() noexcept
    {
        std::lock_guard l(m_runningContexts.lock);
        ++m_runningContexts.count;
    }

    void removeContext() noexcept
    {
        bool needToNotify = false;

        {
            std::unique_lock l(m_runningContexts.lock);
            --m_runningContexts.count;

            if (m_runningContexts.count == 0)
                needToNotify = true;
        }

        if (needToNotify)
            m_runningContexts.cv.notify_all();
    }

    void waitRunningContexts()
    {
        while (m_runningContexts.count > 0)
        {
            ClientTrace2(m_log, "{}.ClientImpl::waitRunningContexts(): there are {} running contexts yet", Er::Format::ptr(this), m_runningContexts.count);

            std::unique_lock l(m_runningContexts.lock);
            m_runningContexts.cv.wait(l, [this]() { return (m_runningContexts.count <= 0); });
        }
    }

    const bool m_grpcReady;
    const std::unique_ptr<erebus::Erebus::Stub> m_stub;
    Er::Log2::ILogger::Ptr m_logRef;
    Er::Log2::ILogger* const m_log;
    const std::uint32_t m_clientId;

    struct PropertyMapping
    {
        std::shared_mutex lock;
        std::vector<Er::PropertyInfo const*> map;
        std::uint32_t version = std::uint32_t(-1);
    };
    
    PropertyMapping m_propertyMapping;

    struct RunningContexts
    {
        std::mutex lock;
        std::condition_variable cv;
        std::int32_t count = 0;
    };

    RunningContexts m_runningContexts;
};


} // namespace {}



ER_GRPC_CLIENT_EXPORT ChannelPtr createChannel(const ChannelSettings& params)
{
    grpc::ChannelArguments args;

    if (params.keepAlive)
    {
        args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 20 * 1000);
        args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10 * 1000);
        args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
    }

    if (params.useTls)
    {
        grpc::SslCredentialsOptions opts;
        opts.pem_root_certs = params.rootCertificates;
        opts.pem_cert_chain = params.certificate;
        opts.pem_private_key = params.privateKey;

        auto channelCreds = grpc::SslCredentials(opts);
        return grpc::CreateCustomChannel(params.endpoint, channelCreds, args);
    }
    else
    {
        return grpc::CreateCustomChannel(params.endpoint, grpc::InsecureChannelCredentials(), args);
    }
}

ER_GRPC_CLIENT_EXPORT IClient::Ptr createClient(ChannelPtr channel, Er::Log2::ILogger::Ptr log)
{
    return std::make_unique<ClientImpl>(std::static_pointer_cast<grpc::Channel>(channel), log);
}

} // namespace Er::Ipc::Grpc {}