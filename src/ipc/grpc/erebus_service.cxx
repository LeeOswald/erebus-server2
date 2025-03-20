#include "erebus_service.hxx"

#include <grpcpp/grpcpp.h>

namespace Erp::Ipc::Grpc
{

ErebusService::~ErebusService()
{
    m_server->Shutdown();
}

ErebusService::ErebusService(const Er::Ipc::Grpc::ServerArgs& params)
    : m_params(params)
{
    grpc::ServerBuilder builder;

    for (auto& ep : m_params.endpoints)
    {
        if (ep.useTls)
        {
            grpc::SslServerCredentialsOptions::PemKeyCertPair keycert = { ep.privateKey, ep.certificate };
            grpc::SslServerCredentialsOptions sslOps(GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY);
            sslOps.pem_root_certs = ep.rootCertificates;
            sslOps.pem_key_cert_pairs.push_back(keycert);
            auto creds = grpc::SslServerCredentials(sslOps);
            builder.AddListeningPort(ep.endpoint, creds);
        }
        else
        {
            // no authentication
            builder.AddListeningPort(ep.endpoint, grpc::InsecureServerCredentials());
        }
    }

    if (m_params.keepAlive)
    {
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 1 * 30 * 1000);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 60 * 1000);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS, 5 * 1000);
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MAX_PING_STRIKES, 5);
    }

    builder.RegisterService(this);

    // finally assemble the server
    auto server = builder.BuildAndStart();
    if (!server)
        ErThrow("Failed to start the gRPC server");

    m_server.swap(server);
}

Er::Ipc::IService::Ptr ErebusService::findService(const std::string& id) const
{
    std::shared_lock l(m_servicesLock);

    auto it = m_services.find(id);
    if (it != m_services.end())
    {
        return it->second;
    }

    return {};
}

Er::PropertyBag ErebusService::unmarshalArgs(const erebus::ServiceRequest* request, Er::IPropertyMapping* mapping, std::string_view context)
{
    Er::PropertyBag bag;

    int count = request->args_size();
    if (count > 0)
    {
        bag.reserve(count);
        
        for (int i = 0; i < count; ++i)
        {
            auto& arg = request->args(i);
            bag.push_back(Erp::Protocol::getProperty(arg, mapping, context));
        }
    }

    return bag;
}

void ErebusService::marshalReplyProps(const Er::PropertyBag& props, erebus::ServiceReply* reply)
{
    if (props.empty())
        return;

    auto out = reply->mutable_props();
    for (auto& prop : props)
    {
        auto mutableProp = out->Add();
        Erp::Protocol::assignProperty(*mutableProp, prop);
    }
}

void ErebusService::marshalException(erebus::ServiceReply* reply, const std::exception& e)
{
    std::string_view what;
    auto msg = e.what();
    if (msg && *msg)
        what = msg;
    else
        what = "std::exception";

    auto exception = reply->mutable_exception();
    *exception->mutable_message() = std::string_view(e.what());
}

void ErebusService::marshalException(erebus::ServiceReply* reply, const Er::Exception& e)
{
    std::string_view what;
    auto msg = e.message();
    if (!msg.empty())
        what = msg;
    else
        what = "Er::Exception";

    auto exception = reply->mutable_exception();
    *exception->mutable_message() = what;

    auto properties = e.properties();
    if (!properties.empty())
    {
        auto mutableProps = exception->mutable_props();
        mutableProps->Reserve(properties.size());

        for (auto& property : properties)
        {
            auto mutableProp = mutableProps->Add();
            Erp ::Protocol::assignProperty(*mutableProp, property);
        }
    }
}

grpc::ServerUnaryReactor* ErebusService::GenericRpc(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request, erebus::ServiceReply* reply)
{
    Er::Log2::debug(m_params.log, "ErebusService::GenericRpc");
    Er::Log2::Indent idt(m_params.log);

    auto reactor = std::make_unique<ReplyUnaryReactor>(m_params.log);
    if (context->IsCancelled()) [[unlikely]]
    {
        Er::Log2::warning(m_params.log, "Request cancelled");
        return reactor.release();
    }

    auto& requestStr = request->request();
    Er::Log2::info(m_params.log, "Req [{}] from {}", requestStr, context->peer());

    auto service = findService(requestStr);
    if (!service) [[unlikely]]
    {
        auto msg = Er::format("No handlers for [{}]", requestStr);
        Er::Log2::writeln(m_params.log, Er::Log2::Level::Error, msg);
        reactor->Finish(grpc::Status(grpc::UNAVAILABLE, msg));
        return reactor.release();
    }

    std::string_view cookie;
    if (request->has_cookie())
        cookie = request->cookie();

    ExceptionMarshaler xcptHandler(m_params.log, *reply);
    try
    {
        auto args = unmarshalArgs(request, this, cookie);
        auto result = service->request(requestStr, cookie, args);
        marshalReplyProps(result, reply);
    }
    catch (...)
    {
        Er::dispatchException(std::current_exception(), xcptHandler);
    }

    reactor->Finish(grpc::Status::OK);
    return reactor.release();
}

grpc::ServerWriteReactor<erebus::ServiceReply>* ErebusService::GenericStream(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request)
{
    Er::Log2::debug(m_params.log, "ErebusService::GenericStream");
    Er::Log2::Indent idt(m_params.log);

    auto reactor = std::make_unique<ReplyStreamWriteReactor>(m_params.log);

    auto& requestStr = request->request();
    Er::Log2::info(m_params.log, "Strm [{}] to {}", requestStr, context->peer());

    auto service = findService(requestStr);
    if (!service)
    {
        auto msg = Er::format("No handlers for [{}]", requestStr);
        Er::Log2::writeln(m_params.log, Er::Log2::Level::Error, msg);
        reactor->Finish(grpc::Status(grpc::UNAVAILABLE, msg));
        return reactor.release();
    }

    std::string_view cookie;
    if (request->has_cookie())
        cookie = request->cookie();

    std::string errorMsg;
    Er::Util::ExceptionLogger xcptLogger(m_params.log);
    try
    {
        auto args = unmarshalArgs(request, this, cookie);
        reactor->Begin(service, requestStr, cookie, args);
        return reactor.release();
    }
    catch (...)
    {
        Er::dispatchException(std::current_exception(), xcptLogger);
        
        errorMsg = xcptLogger.lastError();
    }

    reactor->Finish(grpc::Status(grpc::INTERNAL, errorMsg));
    return reactor.release();
}

grpc::ServerWriteReactor<erebus::PropertyInfo>* ErebusService::GetPropertyMapping(grpc::CallbackServerContext* context, const erebus::Void* request)
{
    Er::Log2::debug(m_params.log, "ErebusService::GetPropertyMapping");
    Er::Log2::Indent idt(m_params.log);

    Er::Log2::info(m_params.log, "Request from {}", context->peer());

    auto reactor = std::make_unique<PropertyInfoStreamWriteReactor>(m_params.log);
    reactor->Begin();
    
    return reactor.release();
}

void ErebusService::registerService(std::string_view request, Er::Ipc::IService::Ptr service)
{
    std::lock_guard l(m_servicesLock);

    std::string id(request);
    auto it = m_services.find(id);
    if (it != m_services.end())
        ErThrow(Er::format("Service for [{}] is already registered", id));

    Er::Log2::info(m_params.log, "Registered service {} for [{}]", Er::Format::ptr(service.get()), id);

    m_services.insert({ std::move(id), service });
}

void ErebusService::unregisterService(Er::Ipc::IService* service)
{
    std::lock_guard l(m_servicesLock);

    bool success = false;
    for (auto it = m_services.begin(); it != m_services.end();)
    {
        if (it->second.get() == service)
        {
            Er::Log2::info(m_params.log, "Unregistered service {}", Er::Format::ptr(service));

            auto next = std::next(it);
            m_services.erase(it);
            it = next;

            success = true;
        }
        else
        {
            ++it;
        }
    }

    if (!success)
        Er::Log2::error(m_params.log, "Service {} is not registered", Er::Format::ptr(service));
}

const Er::PropertyInfo* ErebusService::mapProperty(std::uint32_t id, std::string_view context)
{
    return nullptr;
}

} // namespace Erp::Ipc::Grpc {}


namespace Er::Ipc::Grpc
{
    
IServer::Ptr ER_GRPC_EXPORT create(const ServerArgs& params)
{
    return std::make_unique<Erp::Ipc::Grpc::ErebusService>(params);
}

} // namespace Er::Ipc::Grpc {}