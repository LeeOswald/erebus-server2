#include "protocol.hxx"

#include <erebus/erebus.grpc.pb.h>
#include <grpcpp/grpcpp.h>

#include <erebus/ipc/grpc/grpc_client.hxx>
#include <erebus/system/system/packed_time.hxx>
#include <erebus/system/property_info.hxx>
#include <erebus/system/util/exception_util.hxx>

#include <atomic>
#include <chrono>
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
    case grpc::DEADLINE_EXCEEDED: return Result::DeadlineExceeded;
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
        Er::Log2::debug(m_log, "{}.ClientImpl::~ClientImpl()", Er::Format::ptr(this));
        Er::Log2::Indent idt(m_log);

        ::grpc_shutdown();
    }

    explicit ClientImpl(const ChannelSettings& params, std::shared_ptr<grpc::Channel> channel, Er::Log2::ILogger::Ptr log)
        : m_params(params)
        , m_grpcReady(grpcInit())
        , m_stub(erebus::Erebus::NewStub(channel))
        , m_logRef(log)
        , m_log(log.get())
        , m_cookie(makeNoise(CookieLength))
    {
        Er::Log2::debug(m_log, "{}.ClientImpl::ClientImpl()", Er::Format::ptr(this));
    }

    const Er::PropertyInfo* mapProperty(std::uint32_t id, const std::string& context) override
    {
        {
            std::shared_lock l(m_propertyMapping.lock);

            if (id < m_propertyMapping.map.size())
                return m_propertyMapping.map[id];
        }

        Er::Log2::error(m_log, "{}.ClientImpl::mapProperty({}.[{}]) -> NULL", Er::Format::ptr(this), id, context);
        return nullptr;
    }
    
    void ping(std::size_t payloadSize, IPingCompletion::Ptr handler) override
    {
        Er::Log2::debug(m_log, "{}.ClientImpl::ping()", Er::Format::ptr(this));
        Er::Log2::Indent idt(m_log);

        auto ctx = std::make_shared<PingContext>(m_cookie, payloadSize, handler);
        ctx->context.set_deadline(std::chrono::system_clock::now() + m_params.callTimeout);
        
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
        Er::Log2::debug(m_log, "{}.ClientImpl::getPropertyMapping()", Er::Format::ptr(this));
        Er::Log2::Indent idt(m_log);

        new PropertyMappingStreamReader(this, m_logRef, m_stub.get(), handler);
    }

    void putPropertyMapping(ICompletion::Ptr handler) override
    {
        Er::Log2::debug(m_log, "{}.ClientImpl::putPropertyMapping()", Er::Format::ptr(this));
        Er::Log2::Indent idt(m_log);

        new PropertyMappingStreamWriter(this, m_logRef, m_stub.get(), handler);
    }
    
    void call(std::string_view request, const Er::PropertyBag& args, ICallCompletion::Ptr handler) override
    {
        Er::Log2::debug(m_log, "{}.ClientImpl::call({})", Er::Format::ptr(this), request);
        Er::Log2::Indent idt(m_log);

        auto ctx = std::make_shared<CallContext>(request, args, m_cookie, handler);
        ctx->context.set_deadline(std::chrono::system_clock::now() + m_params.callTimeout);

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
        Er::Log2::debug(m_log, "{}.ClientImpl::stream({})", Er::Format::ptr(this), request);
        Er::Log2::Indent idt(m_log);

        new ServiceReplyStreamReader(this, m_logRef, m_stub.get(), request, args, m_cookie, handler);
    }

private:
    const std::string& cookie() const noexcept
    {
        return m_cookie;
    }

    bool grpcInit() noexcept
    {
        ::grpc_init();
        return true;
    }

    struct PingContext
        : public boost::noncopyable
    {
        IClient::IPingCompletion::Ptr handler;
        Er::System::PackedTime::ValueType started;
        erebus::PingRequest request;
        grpc::ClientContext context;
        erebus::PingReply reply;

        PingContext(const std::string& cookie, std::size_t payloadSize, IClient::IPingCompletion::Ptr handler)
            : handler(handler)
            , started(Er::System::PackedTime::now())
        {
            request.set_cookie(cookie);
            request.set_timestamp(started);
            if (payloadSize)
            {
                auto payload = makeNoise(payloadSize);
                request.set_payload(std::move(payload));
            }
        }
    };

    struct CallContext
        : public boost::noncopyable
    {
        std::string uri;
        IClient::ICallCompletion::Ptr handler;
        erebus::ServiceRequest request;
        grpc::ClientContext context;
        erebus::ServiceReply reply;

        CallContext(std::string_view req, const Er::PropertyBag& args, const std::string& cookie, IClient::ICallCompletion::Ptr handler)
            : uri(req)
            , handler(handler)
        {
            request.set_request(std::string(req));
            request.set_cookie(cookie);
            request.set_mappingver(Erp::propertyMappingVersion());

            for (auto& arg: args)
            {
                auto a = request.add_args();
                Erp::Protocol::assignProperty(*a, arg);
            }
        }
    };
    
    class ServiceReplyStreamReader final
        : public grpc::ClientReadReactor<erebus::ServiceReply>
    {
    public:
        ~ServiceReplyStreamReader()
        {
            Er::Log2::debug(m_log.get(), "{}.ServiceReplyStreamReader::~ServiceReplyStreamReader()", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log.get());
        }

        ServiceReplyStreamReader(ClientImpl* owner, Er::Log2::ILogger::Ptr log, erebus::Erebus::Stub* stub, std::string_view req, const Er::PropertyBag& args, const std::string& cookie, IStreamCompletion::Ptr handler)
            : m_owner(owner)
            , m_log(log)
            , m_uri(req)
            , m_handler(handler)
        {
            Er::Log2::debug(m_log.get(), "{}.ServiceReplyStreamReader::ServiceReplyStreamReader({})", Er::Format::ptr(this), m_uri);
            Er::Log2::Indent idt(m_log.get());

            m_request.set_request(std::string(req));
            m_request.set_cookie(cookie);
            auto ver = Erp::propertyMappingVersion();
            m_request.set_mappingver(ver);

            Er::Log2::debug(m_log.get(), "Sending property mapping v.{}", ver);

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
            Er::Log2::debug(m_log.get(), "{}.ServiceReplyStreamReader::OnReadDone({}, {})", Er::Format::ptr(this), m_uri, ok);
            Er::Log2::Indent idt(m_log.get());

            if (!ok)
                return;

            Er::Util::ExceptionLogger xcptLogger(m_log.get());

            try
            {
                if (m_reply.result() != erebus::CallResult::SUCCESS)
                {
                    auto code = m_reply.result();
                    if (code == erebus::CallResult::PROPERTY_MAPPING_EXPIRED)
                    {
                        Er::Log2::debug(m_log.get(), "Server property mapping expired for {}:{}", m_context.peer(), m_uri);
                        m_handler->handleServerPropertyMappingExpired();
                        
                        m_context.TryCancel();
                        return StartRead(&m_reply);
                    }
                    else if (!m_reply.has_exception())
                    {
                        auto message = Er::format("Unexpected error streaming from {}:{}: {}", m_context.peer(), m_uri, static_cast<int>(code));
                        m_handler->handleTransportError(Er::Result::Failure, std::move(message));

                        m_context.TryCancel();
                        return StartRead(&m_reply);
                    }
                }

                auto remoteMappingVer = m_reply.mappingver();
                auto localMappingVer = m_owner-> m_propertyMapping.version;
                if (remoteMappingVer != localMappingVer)
                {
                    Er::Log2::debug(m_log.get(), "Client property mapping expired for {}:{} (remote v.{} local v.{})", m_context.peer(), m_uri, remoteMappingVer, localMappingVer);
                    m_handler->handleClientPropertyMappingExpired();

                    m_context.TryCancel();
                    return StartRead(&m_reply);
                }

                if (m_reply.has_exception())
                {
                    auto e = m_owner->unmarshalException(m_reply);
                    Er::Log2::error(m_log.get(), "Exception while streaming from {}:{}: {}", m_context.peer(), m_uri, e.what());

                    if (m_handler->handleException(std::move(e)) == Er::CallbackResult::Cancel)
                    {
                        m_context.TryCancel();
                    }

                    return StartRead(&m_reply);
                }
                
                auto reply = m_owner->unmarshal(m_reply);
                if (m_handler->handleFrame(std::move(reply)) == Er::CallbackResult::Cancel)
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
            Er::Log2::debug(m_log.get(), "{}.ServiceReplyStreamReader::OnDone({}, {})", Er::Format::ptr(this), m_uri, int(status.error_code()));
            Er::Log2::Indent idt(m_log.get());

            Er::Util::ExceptionLogger xcptLogger(m_log.get());

            try
            {
                if (!status.ok())
                {
                    auto resultCode = mapGrpcStatus(status.error_code());
                    auto errorMsg = status.error_message();
                    Er::Log2::error(m_log.get(), "Stream from {} terminated with an error: {} ({})", m_context.peer(), resultCode, errorMsg);

                    m_handler->handleTransportError(resultCode, std::move(errorMsg));
                }
            }
            catch (...)
            {
                Er::dispatchException(std::current_exception(), xcptLogger);
            }

            delete this;
        }

    private:
        ClientImpl* const m_owner;
        Er::Log2::ILogger::Ptr m_log;
        std::string m_uri;
        IStreamCompletion::Ptr m_handler;
        erebus::ServiceRequest m_request;
        grpc::ClientContext m_context;
        erebus::ServiceReply m_reply;
    };

    class PropertyMappingStreamReader final
        : public grpc::ClientReadReactor<erebus::GetPropertyMappingReply>
    {
    public:
        ~PropertyMappingStreamReader()
        {
            Er::Log2::debug(m_log.get(), "{}.PropertyMappingStreamReader::~PropertyMappingStreamReader()", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log.get());
        }

        PropertyMappingStreamReader(ClientImpl* owner, Er::Log2::ILogger::Ptr log, erebus::Erebus::Stub* stub, ICompletion::Ptr handler)
            : m_owner(owner)
            , m_log(log)
            , m_handler(handler)
        {
            Er::Log2::debug(m_log.get(), "{}.PropertyMappingStreamReader::PropertyMappingStreamReader()", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log.get());

            stub->async()->GetPropertyMapping(&m_context, &m_request, this);
            StartRead(&m_reply);
            StartCall();
        }

        void OnReadDone(bool ok) override
        {
            Er::Log2::debug(m_log.get(), "{}.PropertyMappingStreamReader::OnReadDone({})", Er::Format::ptr(this), ok);
            Er::Log2::Indent idt(m_log.get());

            if (ok)
            {
                Er::Util::ExceptionLogger xcptLogger(m_log.get());

                try
                {
                    auto version = m_reply.mappingver();

                    auto id = m_reply.mapping().id();
                    auto type = static_cast<Er::PropertyType>(m_reply.mapping().type());
                    auto& name = m_reply.mapping().name();
                    auto& readableName = m_reply.mapping().readable_name();

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
            Er::Log2::debug(m_log.get(), "{}.PropertyMappingStreamReader::OnDone({})", Er::Format::ptr(this), int(status.error_code()));
            Er::Log2::Indent idt(m_log.get());

            Er::Util::ExceptionLogger xcptLogger(m_log.get());

            try
            {
                if (!status.ok())
                {
                    auto resultCode = mapGrpcStatus(status.error_code());
                    auto errorMsg = status.error_message();
                    Er::Log2::error(m_log.get(), "Stream from {} terminated with an error: {} ({})", m_context.peer(), resultCode, errorMsg);

                    m_handler->handleTransportError(resultCode, std::move(errorMsg));
                }
                else
                {
                    m_handler->handleSuccess();
                }
            }
            catch (...)
            {
                Er::dispatchException(std::current_exception(), xcptLogger);
            }

            delete this;
        }

    private:
        ClientImpl* const m_owner;
        Er::Log2::ILogger::Ptr m_log;
        ICompletion::Ptr m_handler;
        erebus::Void m_request;
        grpc::ClientContext m_context;
        erebus::GetPropertyMappingReply m_reply;
    };

    class PropertyMappingStreamWriter final
        : public grpc::ClientWriteReactor<erebus::PutPropertyMappingRequest>
    {
    public:
        ~PropertyMappingStreamWriter()
        {
            Er::Log2::debug(m_log.get(), "{}.PropertyMappingStreamWriter::~PropertyMappingStreamWriter()", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log.get());
        }

        PropertyMappingStreamWriter(ClientImpl* owner, Er::Log2::ILogger::Ptr log, erebus::Erebus::Stub* stub, ICompletion::Ptr handler)
            : m_owner(owner)
            , m_log(log)
            , m_handler(handler)
        {
            Er::Log2::debug(m_log.get(), "{}.PropertyMappingStreamWriter::PropertyMappingStreamWriter()", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log.get());

            m_mappingVer = Er::enumerateProperties(
                [this](const Er::PropertyInfo* pi) -> bool
                {
                    m_mapping.push_back(pi);
                    return true;
                });

            Er::Log2::debug(m_log.get(), "Sending local property mapping v.{}", m_mappingVer);

            stub->async()->PutPropertyMapping(&m_context, &m_reply, this);

            nextWrite();
            StartCall();
        }

        void OnWriteDone(bool ok) override 
        {
            Er::Log2::debug(m_log.get(), "{}.PropertyMappingStreamWriter::OnWriteDone({})", Er::Format::ptr(this), ok);
            Er::Log2::Indent idt(m_log.get());

            if (ok) 
            { 
                nextWrite(); 
            }
        }

        void OnDone(const grpc::Status& status) override 
        {
            Er::Log2::debug(m_log.get(), "{}.PropertyMappingStreamWriter::OnDone({})", Er::Format::ptr(this), static_cast<int>(status.error_code()));
            Er::Log2::Indent idt(m_log.get());

            Er::Util::ExceptionLogger xcptLogger(m_log.get());

            try
            {
                if (!status.ok())
                {
                    auto resultCode = mapGrpcStatus(status.error_code());
                    auto errorMsg = status.error_message();
                    Er::Log2::error(m_log.get(), "Stream to {} terminated with an error: {} ({})", m_context.peer(), resultCode, errorMsg);

                    m_handler->handleTransportError(resultCode, std::move(errorMsg));
                }
                else
                {
                    m_handler->handleSuccess();
                }
            }
            catch (...)
            {
                Er::dispatchException(std::current_exception(), xcptLogger);
            }

            delete this;
        }

    private:
        void nextWrite()
        {
            if (m_nextIndex < m_mapping.size())
            {
                Er::Log2::debug(m_log.get(), "{}.PropertyMappingStreamWriter::nextWrite(#{} of {})", Er::Format::ptr(this), m_nextIndex, m_mapping.size());
                Er::Log2::Indent idt(m_log.get());

                m_request.set_cookie(m_owner->cookie());
                m_request.set_mappingver(m_mappingVer);

                auto prop = m_mapping[m_nextIndex++];

                auto m = m_request.mutable_mapping();
                m->set_id(prop->unique());
                m->set_type(static_cast<std::uint32_t>(prop->type()));
                m->set_name(prop->name());
                m->set_readable_name(prop->readableName());

                StartWrite(&m_request);
            }
            else
            {
                Er::Log2::debug(m_log.get(), "{}.PropertyMappingStreamWriter::nextWrite(): end of stream", Er::Format::ptr(this));
                Er::Log2::Indent idt(m_log.get());

                StartWritesDone();
            }
        }

        ClientImpl* const m_owner;
        Er::Log2::ILogger::Ptr m_log;
        ICompletion::Ptr m_handler;
        erebus::PutPropertyMappingRequest m_request;
        grpc::ClientContext m_context;
        erebus::Void m_reply;
        std::vector<const Er::PropertyInfo*> m_mapping;
        std::uint32_t m_mappingVer = std::uint32_t(-1);
        std::size_t m_nextIndex = 0;
    };

    void completePing(std::shared_ptr<PingContext> ctx, grpc::Status status, std::size_t payloadSize)
    {
        Er::Log2::debug(m_log, "{}.ClientImpl::completePing({})", Er::Format::ptr(this), static_cast<int>(status.error_code()));
        Er::Log2::Indent idt(m_log);

        Er::Util::ExceptionLogger xcptLogger(m_log);

        try
        {
            if (!status.ok())
            {
                auto resultCode = mapGrpcStatus(status.error_code());
                auto errorMsg = status.error_message();
                Er::Log2::error(m_log, "Failed to ping {}: {} ({})", ctx->context.peer(), resultCode, errorMsg);

                ctx->handler->handleTransportError(resultCode, std::move(errorMsg));
            }
            else
            {
                auto finished = Er::System::PackedTime::now();
                auto milliseconds = (finished - ctx->started) / 1000;

                ctx->handler->handleReply(payloadSize, std::chrono::milliseconds(milliseconds));
            }
        }
        catch (...)
        {
            Er::dispatchException(std::current_exception(), xcptLogger);
        }
    }

    void completeCall(std::shared_ptr<CallContext> ctx, grpc::Status status)
    {
        Er::Log2::debug(m_log, "{}.ClientImpl::completeCall({})", Er::Format::ptr(this), static_cast<int>(status.error_code()));
        Er::Log2::Indent idt(m_log);

        Er::Util::ExceptionLogger xcptLogger(m_log);

        try
        {
            if (!status.ok())
            {
                auto resultCode = mapGrpcStatus(status.error_code());
                auto errorMsg = status.error_message();
                Er::Log2::error(m_log, "Failed to call {}:{}: {} ({})", ctx->context.peer(), ctx->uri, resultCode, errorMsg);

                ctx->handler->handleTransportError(resultCode, std::move(errorMsg));

                return;
            }

            if (ctx->reply.result() != erebus::CallResult::SUCCESS)
            {
                auto code = ctx->reply.result();
                if (code == erebus::CallResult::PROPERTY_MAPPING_EXPIRED)
                {
                    Er::Log2::debug(m_log, "Server property mapping expired for {}:{}", ctx->context.peer(), ctx->uri);
                    ctx->handler->handleServerPropertyMappingExpired();
                    return;
                }
                else if (!ctx->reply.has_exception())
                {
                    auto message = Er::format("Unexpected error calling {}:{}: {}", ctx->context.peer(), ctx->uri, static_cast<int>(code));
                    ctx->handler->handleTransportError(Er::Result::Failure, std::move(message));
                    return;
                }
            }

            auto remoteMappingVer = ctx->reply.mappingver();
            auto localMappingVer = m_propertyMapping.version;
            if (remoteMappingVer != localMappingVer)
            {
                Er::Log2::debug(m_log, "Client property mapping expired for {}:{} (remote v.{} local v.{})", ctx->context.peer(), ctx->uri, remoteMappingVer, localMappingVer);
                ctx->handler->handleClientPropertyMappingExpired();
                return;
            }

            if (ctx->reply.has_exception())
            {
                auto e = unmarshalException(ctx->reply);
                Er::Log2::error(m_log, "Failed to call {}:{}: {}", ctx->context.peer(), ctx->uri, e.what());

                ctx->handler->handleException(std::move(e));

                return;
            }
                        
            auto reply = unmarshal(ctx->reply);
            ctx->handler->handleReply(std::move(reply));
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
            unmarshaledException.add(Erp::Protocol::getProperty(prop, this, m_cookie));
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

            bag.push_back(Erp::Protocol::getProperty(prop, this, m_cookie));
        }

        return bag;
    }

    void putPropertyMapping(std::uint32_t version, std::uint32_t id, Er::PropertyType type, const std::string& name, const std::string& readableName)
    {
        Er::Log2::debug(m_log, "{}.ClientImpl::putPropertyMapping(v.{} {} -> {} [{}])", Er::Format::ptr(this), version, id, name, readableName);
        Er::Log2::Indent idt(m_log);

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

    static constexpr size_t CookieLength = 32;

    const ChannelSettings m_params;
    const bool m_grpcReady;
    const std::unique_ptr<erebus::Erebus::Stub> m_stub;
    Er::Log2::ILogger::Ptr m_logRef;
    Er::Log2::ILogger* const m_log;
    const std::string m_cookie;

    struct PropertyMapping
    {
        std::shared_mutex lock;
        std::vector<Er::PropertyInfo const*> map;
        std::uint32_t version = std::uint32_t(-1);
    };
    
    PropertyMapping m_propertyMapping;
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

ER_GRPC_CLIENT_EXPORT IClient::Ptr createClient(const ChannelSettings& params, ChannelPtr channel, Er::Log2::ILogger::Ptr log)
{
    return std::make_unique<ClientImpl>(params, std::static_pointer_cast<grpc::Channel>(channel), log);
}

} // namespace Er::Ipc::Grpc {}