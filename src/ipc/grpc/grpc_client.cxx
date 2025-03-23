#include "protocol.hxx"

#include <erebus/erebus.grpc.pb.h>
#include <grpcpp/grpcpp.h>

#include <erebus/ipc/grpc/grpc_client.hxx>
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

    explicit ClientImpl(std::shared_ptr<grpc::Channel> channel, Er::Log2::ILogger* log)
        : m_grpcReady(grpcInit())
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

    void ping(std::optional<std::chrono::milliseconds> timeout) override
    {
        Er::Log2::debug(m_log, "{}.ClientReadReactor::ping()", Er::Format::ptr(this));
        Er::Log2::Indent idt(m_log);

        auto ctx = std::make_shared<PingContext>(m_cookie);
        if (timeout)
        {
            ctx->context.set_deadline(std::chrono::system_clock::now() + *timeout);
        }

        m_stub->async()->Ping(
            &ctx->context,
            &ctx->request,
            &ctx->reply,
            [this, ctx](grpc::Status status)
            {
                if (!status.ok())
                {
                    Er::Log2::error(m_log, "Failed to ping {}: {} ({})", ctx->context.peer(), mapGrpcStatus(status.error_code()), status.error_message());
                }
                else
                {
                    auto finished = std::chrono::steady_clock::now();
                    auto dura = finished - ctx->started;
                    auto msecs = std::chrono::duration_cast<std::chrono::milliseconds>(dura);
                    Er::Log2::info(m_log, "Pinged {} in {} ms", msecs.count());
                }
            });
    }

    void request(CallId callId, std::string_view request, const Er::PropertyBag& args, IReceiver* receiver, std::optional<std::chrono::milliseconds> timeout) override
    {
        Er::Log2::debug(m_log, "{}.ClientReadReactor::request({}.{})", Er::Format::ptr(this), callId, request);
        Er::Log2::Indent idt(m_log);

        auto ctx = std::make_shared<UnaryCallbackContext>(callId, request, args, m_cookie, receiver);
        if (timeout)
        {
            ctx->context.set_deadline(std::chrono::system_clock::now() + *timeout);
        }

        m_stub->async()->GenericRpc(
            &ctx->context,
            &ctx->request,
            &ctx->reply,
            [this, ctx](grpc::Status status)
            {
                if (!status.ok())
                {
                    ctx->receiver->receive(ctx->callId, mapGrpcStatus(status.error_code()), status.error_message());
                }
                else if (ctx->reply.has_exception())
                {
                    ctx->receiver->receive(ctx->callId, unmarshalException(ctx->reply));
                }
                else
                {
                    ctx->receiver->receive(ctx->callId, unmarshal(ctx->reply));
                }
            });
    }

    void requestStream(CallId callId, std::string_view request, const Er::PropertyBag& args, IStreamReceiver* receiver) override
    {
        Er::Log2::debug(m_log, "{}.ClientReadReactor::requestStream({}.{})", Er::Format::ptr(this), callId, request);
        Er::Log2::Indent idt(m_log);

        new ClientReadReactor(this, m_stub.get(), callId, request, args, receiver);
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
        virtual ~PingContext() = default;

        std::chrono::time_point<std::chrono::steady_clock> started = std::chrono::steady_clock::now();
        erebus::PingRequest request;
        grpc::ClientContext context;
        erebus::PingReply reply;

        PingContext(const std::string& cookie)
        {
            request.set_cookie(cookie);
        }
    };

    struct CallContext
        : public boost::noncopyable
    {
        virtual ~CallContext() = default;

        erebus::ServiceRequest request;
        grpc::ClientContext context;

        CallContext(std::string_view req, const Er::PropertyBag& args, const std::string& cookie)
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

    struct UnaryCallbackContext final
        : public CallContext
    {
        CallId callId;
        erebus::ServiceReply reply;
        IReceiver* receiver;

        UnaryCallbackContext(CallId callId, std::string_view req, const Er::PropertyBag& args, const std::string& cookie, IReceiver* receiver)
            : CallContext(req, args, cookie)
            , callId(callId)
            , receiver(receiver)
        {
        }
    };

    class ClientReadReactor final
        : public grpc::ClientReadReactor<erebus::ServiceReply>
    {
    public:
        ~ClientReadReactor()
        {
            Er::Log2::debug(m_owner->log(), "{}.ClientReadReactor::~ClientReadReactor({})", Er::Format::ptr(this), m_callId);
            Er::Log2::Indent idt(m_owner->log());
        }

        ClientReadReactor(ClientImpl* owner, erebus::Erebus::Stub* stub, CallId callId, std::string_view req, const Er::PropertyBag& args, IStreamReceiver* receiver)
            : m_owner(owner)
            , m_context(req, args, owner->cookie())
            , m_callId(callId)
            , m_receiver(receiver)
        {
            Er::Log2::debug(m_owner->log(), "{}.ClientReadReactor::ClientReadReactor({})", Er::Format::ptr(this), m_callId);
            Er::Log2::Indent idt(m_owner->log());

            stub->async()->GenericStream(&m_context.context, &m_context.request, this);
            StartRead(&m_reply);
            StartCall();
        }

        void OnReadDone(bool ok) override
        {
            Er::Log2::debug(m_owner->log(), "{}.ClientReadReactor::OnReadDone({}, {})", Er::Format::ptr(this), m_callId, ok);
            Er::Log2::Indent idt(m_owner->log());

            if (ok)
            {
                if (m_reply.has_exception())
                {
                    if (m_receiver->receive(m_callId, m_owner->unmarshalException(m_reply)) == IClient::IStreamReceiver::Result::Cancel)
                    {
                        m_context.context.TryCancel();
                    }
                }
                else if (m_receiver->receive(m_callId, m_owner->unmarshal(m_reply)) == IClient::IStreamReceiver::Result::Cancel)
                {
                    m_context.context.TryCancel();
                }

                // we have to drain the completion queue even if we cancel
                StartRead(&m_reply);
            }
        }

        void OnDone(const grpc::Status& status) override
        {
            Er::Log2::debug(m_owner->log(), "{}.ClientReadReactor::OnDone({}, {})", Er::Format::ptr(this), m_callId, int(status.error_code()));
            Er::Log2::Indent idt(m_owner->log());

            if (!status.ok())
            {
                m_receiver->finish(m_callId, mapGrpcStatus(status.error_code()), status.error_message());
            }
            else
            {
                m_receiver->finish(m_callId);
            }

            delete this;
        }

    private:
        ClientImpl* const m_owner;
        CallContext m_context;
        CallId m_callId;
        erebus::ServiceReply m_reply;
        IStreamReceiver* m_receiver;
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

    static const size_t CookieLength = 32;
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

ER_GRPC_CLIENT_EXPORT IClient::Ptr createClient(ChannelPtr channel, Er::Log2::ILogger* log)
{
    return std::make_unique<ClientImpl>(std::static_pointer_cast<grpc::Channel>(channel), log);
}

} // namespace Er::Ipc::Grpc {}