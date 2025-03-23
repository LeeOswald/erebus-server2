#include "protocol.hxx"

#include <erebus/erebus.grpc.pb.h>
#include <grpcpp/grpcpp.h>

#include <erebus/ipc/grpc/grpc_client.hxx>
#include <erebus/system/system/packed_time.hxx>
#include <erebus/system/property_info.hxx>

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

    void ping(std::size_t payloadSize, IPingCompletion* handler, void* context) override
    {
        Er::Log2::debug(m_log, "{}.ClientReadReactor::ping()", Er::Format::ptr(this));
        Er::Log2::Indent idt(m_log);

        auto ctx = std::make_shared<PingContext>(m_cookie, payloadSize, handler, context);
        ctx->context.set_deadline(std::chrono::system_clock::now() + m_params.callTimeout);
        
        m_stub->async()->Ping(
            &ctx->context,
            &ctx->request,
            &ctx->reply,
            [this, ctx](grpc::Status status)
            {
                if (!status.ok())
                {
                    auto resultCode = mapGrpcStatus(status.error_code());
                    auto errorMsg = status.error_message();
                    Er::Log2::error(m_log, "Failed to ping {}: {} ({})", ctx->context.peer(), resultCode, errorMsg);

                    ctx->handler->handleTransportError(ctx->userCtx, resultCode, std::move(errorMsg));
                }
                else
                {
                    auto finished = Er::System::PackedTime::now();
                    auto milliseconds = (finished - ctx->started) / 1000;
                    
                    ctx->handler->handleReply(ctx->userCtx, std::chrono::milliseconds(milliseconds));
                }
            });
    }

    void call(std::string_view request, const Er::PropertyBag& args, ICallCompletion* handler, void* context) override
    {
        Er::Log2::debug(m_log, "{}.ClientReadReactor::call({})", Er::Format::ptr(this), request);
        Er::Log2::Indent idt(m_log);

        auto ctx = std::make_shared<CallContext>(request, args, m_cookie, handler, context);
        ctx->context.set_deadline(std::chrono::system_clock::now() + m_params.callTimeout);

        m_stub->async()->GenericRpc(
            &ctx->context,
            &ctx->request,
            &ctx->reply,
            [this, ctx](grpc::Status status)
            {
                if (!status.ok())
                {
                    auto resultCode = mapGrpcStatus(status.error_code());
                    auto errorMsg = status.error_message();
                    Er::Log2::error(m_log, "Failed to call {}:{}: {} ({})", ctx->context.peer(), ctx->uri, resultCode, errorMsg);

                    ctx->handler->handleTransportError(ctx->userCtx, resultCode, std::move(errorMsg));
                }
                else if (ctx->reply.has_exception())
                {
                    auto e = unmarshalException(ctx->reply);
                    Er::Log2::error(m_log, "Failed to call {}:{}", ctx->context.peer(), ctx->uri, e.what());

                    ctx->handler->handleException(ctx->uri, ctx->userCtx, std::move(e));
                }
                else
                {
                    auto reply = unmarshal(ctx->reply);
                    ctx->handler->handleReply(ctx->uri, ctx->userCtx, std::move(reply));
                }
            });
    }

    void stream(std::string_view request, const Er::PropertyBag& args, IStreamCompletion* handler, void* context) override
    {
        Er::Log2::debug(m_log, "{}.ClientReadReactor::stream({})", Er::Format::ptr(this), request);
        Er::Log2::Indent idt(m_log);

        new ClientReadReactor(this, m_stub.get(), request, args, handler, context);
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
        IClient::IPingCompletion* handler;
        void* userCtx;
        Er::System::PackedTime::ValueType started;
        erebus::PingRequest request;
        grpc::ClientContext context;
        erebus::PingReply reply;

        PingContext(const std::string& cookie, std::size_t payloadSize, IClient::IPingCompletion* handler, void* userCtx)
            : handler(handler)
            , userCtx(userCtx)
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
        IClient::ICallCompletion* handler;
        void* userCtx;
        erebus::ServiceRequest request;
        grpc::ClientContext context;
        erebus::ServiceReply reply;

        CallContext(std::string_view req, const Er::PropertyBag& args, const std::string& cookie, IClient::ICallCompletion* handler, void* userCtx)
            : uri(req)
            , handler(handler)
            , userCtx(userCtx)
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
    
    class ClientReadReactor final
        : public grpc::ClientReadReactor<erebus::ServiceReply>
    {
    public:
        ~ClientReadReactor()
        {
            Er::Log2::debug(m_owner->log(), "{}.ClientReadReactor::~ClientReadReactor()", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_owner->log());
        }

        ClientReadReactor(ClientImpl* owner, erebus::Erebus::Stub* stub, std::string_view req, const Er::PropertyBag& args, IStreamCompletion* handler, void* context)
            : m_owner(owner)
            , m_uri(req)
            , m_handler(handler)
            , m_userCtx(context)
        {
            Er::Log2::debug(m_owner->log(), "{}.ClientReadReactor::ClientReadReactor({})", Er::Format::ptr(this), m_uri);
            Er::Log2::Indent idt(m_owner->log());

            stub->async()->GenericStream(&m_context, &m_request, this);
            StartRead(&m_reply);
            StartCall();
        }

        void OnReadDone(bool ok) override
        {
            Er::Log2::debug(m_owner->log(), "{}.ClientReadReactor::OnReadDone({}, {})", Er::Format::ptr(this), m_uri, ok);
            Er::Log2::Indent idt(m_owner->log());

            if (ok)
            {
                if (m_reply.has_exception())
                {
                    auto e = m_owner->unmarshalException(m_reply);
                    Er::Log2::error(m_owner->log(), "Failed to call {}:{}", m_context.peer(), m_uri, e.what());

                    if (m_handler->handleException(m_uri, m_userCtx, std::move(e)) == IClient::IStreamCompletion::Should::Cancel)
                    {
                        m_context.TryCancel();
                    }
                }
                else
                {
                    auto reply = m_owner->unmarshal(m_reply);
                    if (m_handler->handleFrame(m_uri, m_userCtx, std::move(reply)) == IClient::IStreamCompletion::Should::Cancel)
                    {
                        m_context.TryCancel();
                    }
                }

                // we have to drain the completion queue even if we cancel
                StartRead(&m_reply);
            }
        }

        void OnDone(const grpc::Status& status) override
        {
            Er::Log2::debug(m_owner->log(), "{}.ClientReadReactor::OnDone({}, {})", Er::Format::ptr(this), m_uri, int(status.error_code()));
            Er::Log2::Indent idt(m_owner->log());

            if (!status.ok())
            {
                auto resultCode = mapGrpcStatus(status.error_code());
                auto errorMsg = status.error_message();
                Er::Log2::error(m_owner->log(), "Stream from {} terminated with an error: {} ({})", m_context.peer(), resultCode, errorMsg);

                m_handler->handleTransportError(m_userCtx, resultCode, std::move(errorMsg));
            }
            else
            {
                m_handler->handleEndOfStream(m_uri, m_userCtx);
            }

            delete this;
        }

    private:
        ClientImpl* const m_owner;
        std::string m_uri;
        IStreamCompletion* m_handler;
        void* m_userCtx;
        erebus::ServiceRequest m_request;
        grpc::ClientContext m_context;
        erebus::ServiceReply m_reply;
    };

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

    ChannelSettings m_params;
    bool m_grpcReady;
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