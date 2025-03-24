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
        ::grpc_shutdown();
    }

    explicit ClientImpl(const ChannelSettings& params, std::shared_ptr<grpc::Channel> channel, Er::Log2::ILogger* log)
        : m_params(params)
        , m_grpcReady(grpcInit())
        , m_stub(erebus::Erebus::NewStub(channel))
        , m_log(log)
        , m_cookie(makeNoise(CookieLength))
    {
    }

    const Er::PropertyInfo* mapProperty(std::uint32_t id, const std::string& context) override
    {
        std::shared_lock l(m_propertyMapping.lock);
                
        if (id < m_propertyMapping.map.size())
            return m_propertyMapping.map[id];

        return nullptr;
    }

    void adddPropertyMapping(const Er::PropertyInfo* pi) override
    {
        std::unique_lock l(m_propertyMapping.lock);

        auto index = pi->unique();
        if (m_propertyMapping.map.size() <= index)
            m_propertyMapping.map.resize(index + 1);

        m_propertyMapping.map[index] = pi;
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
            [this, ctx](grpc::Status status)
            {
                completePing(ctx, status);
            });
    }

    void getPropertyMapping(IGetPropertyMappingCompletion::Ptr handler) override
    {
        Er::Log2::debug(m_log, "{}.ClientImpl::getPropertyMapping({})", Er::Format::ptr(this));
        Er::Log2::Indent idt(m_log);

        new PropertyMappingStreamReader(this, m_stub.get(), handler);
    }

    void putPropertyMapping(IPutPropertyMappingCompletion::Ptr handler) override
    {
        Er::Log2::debug(m_log, "{}.ClientImpl::putPropertyMapping({})", Er::Format::ptr(this));
        Er::Log2::Indent idt(m_log);

        new PropertyMappingStreamWriter(this, m_stub.get(), handler);
    }
    
    void call(std::string_view request, const Er::PropertyBag& args, ICallCompletion::Ptr handler) override
    {
        Er::Log2::debug(m_log, "{}.ClientImpl::call({})", Er::Format::ptr(this), request);
        Er::Log2::Indent idt(m_log);

        auto ctx = std::make_shared<CallContext>(request, args, m_cookie, handler);
        ctx->context.set_deadline(std::chrono::system_clock::now() + m_params.callTimeout);

        m_stub->async()->GenericRpc(
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

        new ServiceReplyStreamReader(this, m_stub.get(), request, args, handler);
    }

private:
    Er::Log2::ILogger* log() const noexcept
    {
        return m_log;
    }

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
            auto payload = makeNoise(payloadSize);
            request.set_payload(std::move(payload));
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
            Er::Log2::debug(m_owner->log(), "{}.ServiceReplyStreamReader::~ServiceReplyStreamReader()", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_owner->log());
        }

        ServiceReplyStreamReader(ClientImpl* owner, erebus::Erebus::Stub* stub, std::string_view req, const Er::PropertyBag& args, IStreamCompletion::Ptr handler)
            : m_owner(owner)
            , m_uri(req)
            , m_handler(handler)
        {
            Er::Log2::debug(m_owner->log(), "{}.ServiceReplyStreamReader::ServiceReplyStreamReader({})", Er::Format::ptr(this), m_uri);
            Er::Log2::Indent idt(m_owner->log());

            stub->async()->GenericStream(&m_context, &m_request, this);
            StartRead(&m_reply);
            StartCall();
        }

        void OnReadDone(bool ok) override
        {
            Er::Log2::debug(m_owner->log(), "{}.ServiceReplyStreamReader::OnReadDone({}, {})", Er::Format::ptr(this), m_uri, ok);
            Er::Log2::Indent idt(m_owner->log());

            if (ok)
            {
                Er::Util::ExceptionLogger xcptLogger(m_owner->log());

                try
                {
                    if (m_reply.has_exception())
                    {
                        auto e = m_owner->unmarshalException(m_reply);
                        Er::Log2::error(m_owner->log(), "Failed to call {}:{}", m_context.peer(), m_uri, e.what());

                        if (m_handler->handleException(std::move(e)) == Er::CallbackResult::Cancel)
                        {
                            m_context.TryCancel();
                        }
                    }
                    else
                    {
                        auto reply = m_owner->unmarshal(m_reply);
                        if (m_handler->handleFrame(std::move(reply)) == Er::CallbackResult::Cancel)
                        {
                            m_context.TryCancel();
                        }
                    }
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
            Er::Log2::debug(m_owner->log(), "{}.ServiceReplyStreamReader::OnDone({}, {})", Er::Format::ptr(this), m_uri, int(status.error_code()));
            Er::Log2::Indent idt(m_owner->log());

            Er::Util::ExceptionLogger xcptLogger(m_owner->log());

            try
            {
                if (!status.ok())
                {
                    auto resultCode = mapGrpcStatus(status.error_code());
                    auto errorMsg = status.error_message();
                    Er::Log2::error(m_owner->log(), "Stream from {} terminated with an error: {} ({})", m_context.peer(), resultCode, errorMsg);

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
        std::string m_uri;
        IStreamCompletion::Ptr m_handler;
        erebus::ServiceRequest m_request;
        grpc::ClientContext m_context;
        erebus::ServiceReply m_reply;
    };

    class PropertyMappingStreamReader final
        : public grpc::ClientReadReactor<erebus::PropertyInfo>
    {
    public:
        ~PropertyMappingStreamReader()
        {
            Er::Log2::debug(m_owner->log(), "{}.PropertyMappingStreamReader::~PropertyMappingStreamReader()", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_owner->log());
        }

        PropertyMappingStreamReader(ClientImpl* owner, erebus::Erebus::Stub* stub, IGetPropertyMappingCompletion::Ptr handler)
            : m_owner(owner)
            , m_handler(handler)
        {
            Er::Log2::debug(m_owner->log(), "{}.PropertyMappingStreamReader::PropertyMappingStreamReader()", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_owner->log());

            stub->async()->GetPropertyMapping(&m_context, &m_request, this);
            StartRead(&m_reply);
            StartCall();
        }

        void OnReadDone(bool ok) override
        {
            Er::Log2::debug(m_owner->log(), "{}.PropertyMappingStreamReader::OnReadDone({})", Er::Format::ptr(this), ok);
            Er::Log2::Indent idt(m_owner->log());

            if (ok)
            {
                Er::Util::ExceptionLogger xcptLogger(m_owner->log());

                try
                {
                    auto id = m_reply.id();
                    auto type = static_cast<Er::PropertyType>(m_reply.type());
                    auto& name = m_reply.name();
                    auto& readableName = m_reply.readable_name();

                    if (m_handler->handleProperty(id, type, name, readableName) == Er::CallbackResult::Cancel)
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
        }

        void OnDone(const grpc::Status& status) override
        {
            Er::Log2::debug(m_owner->log(), "{}.PropertyMappingStreamReader::OnDone({}, {})", Er::Format::ptr(this), int(status.error_code()));
            Er::Log2::Indent idt(m_owner->log());

            Er::Util::ExceptionLogger xcptLogger(m_owner->log());

            try
            {
                if (!status.ok())
                {
                    auto resultCode = mapGrpcStatus(status.error_code());
                    auto errorMsg = status.error_message();
                    Er::Log2::error(m_owner->log(), "Stream from {} terminated with an error: {} ({})", m_context.peer(), resultCode, errorMsg);

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
        IGetPropertyMappingCompletion::Ptr m_handler;
        erebus::Void m_request;
        grpc::ClientContext m_context;
        erebus::PropertyInfo m_reply;
    };

    class PropertyMappingStreamWriter final
        : public grpc::ClientWriteReactor<erebus::PutPropertyMappingRequest>
    {
    public:
        ~PropertyMappingStreamWriter()
        {
            Er::Log2::debug(m_owner->log(), "{}.PropertyMappingStreamWriter::~PropertyMappingStreamWriter()", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_owner->log());
        }

        PropertyMappingStreamWriter(ClientImpl* owner, erebus::Erebus::Stub* stub, IPutPropertyMappingCompletion::Ptr handler)
            : m_owner(owner)
            , m_handler(handler)
        {
            Er::Log2::debug(m_owner->log(), "{}.PropertyMappingStreamWriter::PropertyMappingStreamWriter()", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_owner->log());

            Er::enumerateProperties(
                [this](const Er::PropertyInfo* pi) -> bool
                {
                    m_mapping.push_back(pi);
                    return true;
                });

            stub->async()->PutPropertyMapping(&m_context, &m_reply, this);

            nextWrite();
            StartCall();
        }

        void OnWriteDone(bool ok) override 
        {
            Er::Log2::debug(m_owner->log(), "{}.PropertyMappingStreamWriter::OnWriteDone({})", Er::Format::ptr(this), ok);
            Er::Log2::Indent idt(m_owner->log());

            if (ok) 
            { 
                nextWrite(); 
            }
        }

        void OnDone(const grpc::Status& status) override 
        {
            Er::Log2::debug(m_owner->log(), "{}.PropertyMappingStreamWriter::OnWriteDone({})", Er::Format::ptr(this), static_cast<int>(status.error_code()));
            Er::Log2::Indent idt(m_owner->log());

            Er::Util::ExceptionLogger xcptLogger(m_owner->log());

            try
            {
                if (!status.ok())
                {
                    auto resultCode = mapGrpcStatus(status.error_code());
                    auto errorMsg = status.error_message();
                    Er::Log2::error(m_owner->log(), "Stream to {} terminated with an error: {} ({})", m_context.peer(), resultCode, errorMsg);

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
        void nextWrite()
        {
            ErAssert(m_nextIndex < m_mapping.size());

            Er::Log2::debug(m_owner->log(), "{}.PropertyMappingStreamWriter::nextWrite()", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_owner->log());

            m_request.set_cookie(m_owner->cookie());

            auto prop = m_mapping[m_nextIndex++];

            auto m = m_request.mutable_mapping();
            m->set_id(prop->unique());
            m->set_type(static_cast<std::uint32_t>(prop->type()));
            m->set_name(prop->name());
            m->set_readable_name(prop->readableName());

            StartWrite(&m_request);
        }

        ClientImpl* const m_owner;
        IPutPropertyMappingCompletion::Ptr m_handler;
        erebus::PutPropertyMappingRequest m_request;
        grpc::ClientContext m_context;
        erebus::Void m_reply;
        std::vector<const Er::PropertyInfo*> m_mapping;
        std::size_t m_nextIndex = 0;
    };

    void completePing(std::shared_ptr<PingContext> ctx, grpc::Status status)
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

                ctx->handler->handleReply(std::chrono::milliseconds(milliseconds));
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
            }
            else if (ctx->reply.has_exception())
            {
                auto e = unmarshalException(ctx->reply);
                Er::Log2::error(m_log, "Failed to call {}:{}", ctx->context.peer(), ctx->uri, e.what());

                ctx->handler->handleException(std::move(e));
            }
            else
            {
                auto reply = unmarshal(ctx->reply);
                ctx->handler->handleReply(std::move(reply));
            }
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
    Er::Log2::ILogger* const m_log;
    const std::string m_cookie;

    struct
    {
        std::shared_mutex lock;
        std::vector<Er::PropertyInfo const*> map;
    } m_propertyMapping;
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

ER_GRPC_CLIENT_EXPORT IClient::Ptr createClient(const ChannelSettings& params, ChannelPtr channel, Er::Log2::ILogger* log)
{
    return std::make_unique<ClientImpl>(params, std::static_pointer_cast<grpc::Channel>(channel), log);
}

} // namespace Er::Ipc::Grpc {}