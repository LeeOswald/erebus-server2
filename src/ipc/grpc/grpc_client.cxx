#include <erebus/erebus.grpc.pb.h>
#include <grpcpp/grpcpp.h>

#include "property_mapping.hxx"
#include "protocol.hxx"
#include "trace.hxx"

#include <erebus/ipc/grpc/grpc_client.hxx>
#include <erebus/system/system/packed_time.hxx>
#include <erebus/system/util/exception_util.hxx>
#include <erebus/system/util/null_mutex.hxx>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>

#include <boost/noncopyable.hpp>


namespace Er::Ipc::Grpc
{

namespace
{

class ClientImpl final
    : public IClient
    , public boost::noncopyable
{
public:
    ~ClientImpl()
    {
        ClientTrace2(m_log.get(), "{}.ClientImpl::~ClientImpl()", Er::Format::ptr(this));

        waitRunningContexts();

        ::grpc_shutdown();
    }

    explicit ClientImpl(std::shared_ptr<grpc::Channel> channel, Er::Log2::ILogger::Ptr log)
        : m_grpcReady(grpcInit())
        , m_stub(erebus::Erebus::NewStub(channel))
        , m_log(log)
        , m_clientId(makeClientId())
    {
        ClientTrace2(m_log.get(), "{}.ClientImpl::ClientImpl()", Er::Format::ptr(this));
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

    struct CallContext final
        : public ContextBase
    {
        ~CallContext()
        {
            ClientTrace2(m_log, "{}.CallContext::~CallContext()", Er::Format::ptr(this));
        }

        CallContext(ClientImpl* owner, Er::Log2::ILogger* log, std::string_view req, const Er::PropertyBag& args, ClientId clientId, IClient::ICallCompletion::Ptr handler)
            : ContextBase(owner, log)
            , uri(req)
            , handler(handler)
        {
            ClientTrace2(m_log, "{}.CallContext::CallContext(req={} clientId={})", Er::Format::ptr(this), req, clientId);

            request.set_request(std::string(req));
            request.set_clientid(clientId);
            request.set_timestamp(Er::System::PackedTime::now());

            auto& localProps = owner->m_localProps;

            for (auto& arg: args)
            {
                // send PropertyInfo if necessary
                auto id = arg.unique();
                auto info = arg.info();
                if (localProps.addIfMissing(id, info))
                {
                    auto m = request.add_mapping();
                    Erp::Protocol::assignPropertyInfo(*m, info);
                }

                // send actual property
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
            ClientId clientId, 
            IStreamCompletion::Ptr handler
        )
            : ContextBase(owner, log)
            , m_uri(req)
            , m_handler(handler)
        {
            ClientTraceIndent2(m_log, "{}.ServiceReplyStreamReader::ServiceReplyStreamReader(clientId={})", Er::Format::ptr(this), m_uri, clientId);

            m_request.set_request(std::string(req));
            m_request.set_clientid(clientId);
            m_request.set_timestamp(Er::System::PackedTime::now());

            auto& localProps = owner->m_localProps;

            for (auto& arg : args)
            {
                // send PropertyInfo if necessary
                auto id = arg.unique();
                auto info = arg.info();
                if (localProps.addIfMissing(id, info))
                {
                    // new one just added
                    auto m = m_request.add_mapping();
                    Erp::Protocol::assignPropertyInfo(*m, info);
                }

                // send actual property
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
                if (m_reply.has_exception())
                {
                    // server method threw an exception
                    auto e = m_owner->unmarshalException(m_reply);
                    ErLogError2(m_log, "Exception while streaming from {}:{}: {}", m_context.peer(), m_uri, e.what());

                    m_handler->onException(std::move(e));

                    m_context.TryCancel();
                }
                else
                {
                    // normal stream frame
                    IClient::Timings timings;
                    if (m_reply.has_timestamp())
                    {
                        auto start = m_reply.timestamp();
                        auto end = Er::System::PackedTime::now();
                        timings.roundTripTime = std::chrono::microseconds(end - start);
                    }

                    if (m_reply.has_duration())
                    {
                        timings.processingTime = std::chrono::microseconds(m_reply.duration());
                    }

                    auto reply = m_owner->unmarshalProps(m_reply);
                    if (m_handler->onFrame(std::move(reply), timings) == Er::CallbackResult::Cancel)
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

        void OnDone(const grpc::Status& status) override
        {
            {
                ClientTraceIndent2(m_log, "{}.ServiceReplyStreamReader::OnDone({}, {})", Er::Format::ptr(this), m_uri, int(status.error_code()));

                Er::Util::ExceptionLogger xcptLogger(m_log);

                try
                {
                    if (!status.ok())
                    {
                        auto resultCode = Erp::Protocol::mapGrpcStatus(status.error_code());
                        auto errorMsg = status.error_message();
                        ErLogError2(m_log, "Stream from {} terminated with an error: {} ({})", m_context.peer(), resultCode, errorMsg);

                        m_handler->onTransportError(resultCode, std::move(errorMsg));
                    }

                    
                }
                catch (...)
                {
                    Er::dispatchException(std::current_exception(), xcptLogger);
                }
            }
            
            m_handler->done();

            delete this;
        }

    private:
        std::string m_uri;
        IStreamCompletion::Ptr m_handler;
        erebus::ServiceRequest m_request;
        grpc::ClientContext m_context;
        erebus::ServiceReply m_reply;
    };

    void call(std::string_view request, const Er::PropertyBag& args, ICallCompletion::Ptr handler, std::chrono::milliseconds timeout) override
    {
        ClientTraceIndent2(m_log.get(), "{}.ClientImpl::call({})", Er::Format::ptr(this), request);

        auto ctx = std::make_shared<CallContext>(this, m_log.get(), request, args, m_clientId, handler);
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
        ClientTraceIndent2(m_log.get(), "{}.ClientImpl::stream({})", Er::Format::ptr(this), request);

        new ServiceReplyStreamReader(this, m_log.get(), m_stub.get(), request, args, m_clientId, handler);
    }

    void completeCall(std::shared_ptr<CallContext> ctx, grpc::Status status)
    {
        ClientTraceIndent2(m_log.get(), "{}.ClientImpl::completeCall({})", Er::Format::ptr(this), static_cast<int>(status.error_code()));

        Er::Util::ExceptionLogger xcptLogger(m_log.get());

        try
        {
            if (!status.ok())
            {
                // transport failure or something
                auto resultCode = Erp::Protocol::mapGrpcStatus(status.error_code());
                auto errorMsg = status.error_message();
                ErLogError2(m_log.get(), "Failed to call {}:{}: {} ({})", ctx->context.peer(), ctx->uri, resultCode, errorMsg);

                ctx->handler->onTransportError(resultCode, std::move(errorMsg));
            }
            else
            {
                unmarshalPropertyInfos(ctx->reply);

                if (ctx->reply.has_exception())
                {
                    // server method threw an exception
                    auto e = unmarshalException(ctx->reply);
                    ErLogError2(m_log.get(), "Failed to call {}:{}: {}", ctx->context.peer(), ctx->uri, e.what());

                    ctx->handler->onException(std::move(e));
                }
                else
                {
                    // call succeeded
                    IClient::Timings timings;
                    if (ctx->reply.has_timestamp())
                    {
                        auto start = ctx->reply.timestamp();
                        auto end = Er::System::PackedTime::now();
                        timings.roundTripTime = std::chrono::microseconds(end - start);
                    }

                    if (ctx->reply.has_duration())
                    {
                        timings.processingTime = std::chrono::microseconds(ctx->reply.duration());
                    }

                    auto reply = unmarshalProps(ctx->reply);
                    ctx->handler->onReply(std::move(reply), timings);
                }
            }
        }
        catch (...)
        {
            Er::dispatchException(std::current_exception(), xcptLogger);
        }

        ctx->handler->done();
    }

    Er::PropertyInfo const* unmarshalPropertyInfo(const erebus::PropertyInfo& source)
    {
        auto id = source.id();
        if (id > HighestPropertyId)
            ErThrow(Er::format("Property ID {} exceeds limit", id));

        auto info = m_remoteProps.lookup(id);
        if (!info)
        {
            auto type = static_cast<Er::PropertyType>(source.type());
            auto& name = source.name();
            auto& readableName = source.readablename();

            info = Erp::allocateTransientProperty(type, name, readableName);

            m_remoteProps.add(id, info);
        }

        return info;
    }

    void unmarshalPropertyInfos(const erebus::ServiceReply& reply)
    {
        auto count = reply.mapping_size();
        if (!count)
            return;

        for (decltype(count) i = 0; i < count; ++i)
        {
            auto& pi = reply.mapping(i);

            unmarshalPropertyInfo(pi);
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
            auto info = m_remoteProps.lookup(prop.id());
            if (!info)
            {
                ErLogError2(m_log.get(), "Unknown property id {} of type {} in exception returned by server", prop.id(), Erp::Protocol::propTypeToString(prop));
            }
            else
            {
                unmarshaledException.add(Erp::Protocol::getProperty(prop, info));
            }
        }

        return unmarshaledException;
    }

    Er::PropertyBag unmarshalProps(const erebus::ServiceReply& reply)
    {
        Er::PropertyBag bag;
        int count = reply.props_size();
        for (int i = 0; i < count; ++i)
        {
            auto& prop = reply.props(i);
            auto info = m_remoteProps.lookup(prop.id());
            if (!info)
            {
                ErLogError2(m_log.get(), "Unknown property id {} of type {} in server reply", prop.id(), Erp::Protocol::propTypeToString(prop));
            }
            else
            {
                bag.push_back(Erp::Protocol::getProperty(prop, info));
            }
        }

        return bag;
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
            ClientTrace2(m_log.get(), "{}.ClientImpl::waitRunningContexts(): there are {} running contexts yet", Er::Format::ptr(this), m_runningContexts.count);

            std::unique_lock l(m_runningContexts.lock);
            m_runningContexts.cv.wait(l, [this]() { return (m_runningContexts.count <= 0); });
        }
    }

    static constexpr Er::PropertyInfo::Unique HighestPropertyId = 65535;

    const bool m_grpcReady;
    const std::unique_ptr<erebus::Erebus::Stub> m_stub;
    Er::Log2::ILogger::Ptr m_log;
    const ClientId m_clientId;
    Erp::PropertyMapping<Er::Util::NullSharedMutex> m_localProps;   // our props remote peer is already aware of
    Erp::PropertyMapping<Er::Util::NullSharedMutex> m_remoteProps;  // remote peer's props we're already aware of

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