#include "erebus_service.hxx"

#include <grpcpp/grpcpp.h>

namespace Erp::Ipc::Grpc
{

ErebusService::~ErebusService()
{
    m_server->Shutdown();

    ::grpc_shutdown();
}

ErebusService::ErebusService(const Er::Ipc::Grpc::ServerArgs& params)
    : m_params(params)
    , m_propertyMappings(std::chrono::seconds(600)) // 10 mins
{
    ::grpc_init();

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
    std::shared_lock l(m_services.lock);

    auto it = m_services.map.find(id);
    if (it != m_services.map.end())
    {
        return it->second;
    }

    return {};
}

Er::PropertyBag ErebusService::unmarshalArgs(const erebus::ServiceRequest* request, const std::string& context)
{
    Er::PropertyBag bag;

    int count = request->args_size();
    if (count > 0)
    {
        bag.reserve(count);
        
        for (int i = 0; i < count; ++i)
        {
            auto& arg = request->args(i);
            bag.push_back(Erp::Protocol::getProperty(arg, this, context));
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

grpc::ServerUnaryReactor* ErebusService::Ping(grpc::CallbackServerContext* context, const erebus::PingRequest* request, erebus::PingReply* reply)
{
    Er::Log2::debug(m_params.log, "ErebusService::GenericRpc");
    Er::Log2::Indent idt(m_params.log);

    auto reactor = std::make_unique<ReplyUnaryReactor>(m_params.log);
    if (context->IsCancelled()) [[unlikely]]
    {
        Er::Log2::warning(m_params.log, "Request cancelled");
        return reactor.release();
    }

    auto timestamp = request->timestamp();
    reply->set_timestamp(timestamp);

    auto& payload = request->payload();
    reply->set_payload(payload);
    
    auto& cookie = request->cookie();
    Er::Log2::info(m_params.log, "Ping with {} bytes of data from {}:{}", payload.length(), context->peer(), cookie);

    // touch session data
    [[maybe_unused]] auto session = m_propertyMappings.get(cookie);

    reactor->Finish(grpc::Status::OK);
    return reactor.release();
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
        reactor->Finish(grpc::Status(grpc::UNIMPLEMENTED, msg));
        return reactor.release();
    }

    static std::string empty;
    const std::string& cookie = request->has_cookie() ? request->cookie() : empty;

    ExceptionMarshaler xcptHandler(m_params.log, *reply);
    try
    {
        if ((request->args_size() > 0) && (cookie.empty() || !propertyMappingExists(cookie)))
        {
            reply->set_result(erebus::CallResult::PROPERTY_MAPPING_EXPIRED);
        }
        else
        {
            auto args = unmarshalArgs(request, cookie);
            auto result = service->request(requestStr, cookie, args);
            marshalReplyProps(result, reply);

            reply->set_result(erebus::SUCCESS);
        }
    }
    catch (...)
    {
        Er::dispatchException(std::current_exception(), xcptHandler);

        reply->set_result(erebus::FAILURE);
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
        reactor->Finish(grpc::Status(grpc::UNIMPLEMENTED, msg));
        return reactor.release();
    }

    static std::string empty;
    const std::string& cookie = request->has_cookie() ? request->cookie() : empty;

    std::string errorMsg;
    Er::Util::ExceptionLogger xcptLogger(m_params.log);
    try
    {
        auto args = unmarshalArgs(request, cookie);
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

grpc::ServerReadReactor<erebus::PutPropertyMappingRequest>* ErebusService::PutPropertyMapping(grpc::CallbackServerContext* context, ::erebus::Void* response)
{
    Er::Log2::debug(m_params.log, "ErebusService::PutPropertyMapping");
    Er::Log2::Indent idt(m_params.log);

    Er::Log2::info(m_params.log, "Request from {}", context->peer());

    auto reactor = std::make_unique<PutPropertyMappingStreamReadReactor>(m_params.log, this);
    return reactor.release();
}

void ErebusService::registerService(std::string_view request, Er::Ipc::IService::Ptr service)
{
    std::lock_guard l(m_services.lock);

    std::string id(request);
    auto it = m_services.map.find(id);
    if (it != m_services.map.end())
        ErThrow(Er::format("Service for [{}] is already registered", id));

    Er::Log2::info(m_params.log, "Registered service {} for [{}]", Er::Format::ptr(service.get()), id);

    m_services.map.insert({ std::move(id), service });
}

void ErebusService::unregisterService(Er::Ipc::IService* service)
{
    std::lock_guard l(m_services.lock);

    bool success = false;
    for (auto it = m_services.map.begin(); it != m_services.map.end();)
    {
        if (it->second.get() == service)
        {
            Er::Log2::info(m_params.log, "Unregistered service {}", Er::Format::ptr(service));

            auto next = std::next(it);
            m_services.map.erase(it);
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

const Er::PropertyInfo* ErebusService::mapProperty(std::uint32_t id, const std::string& context)
{
    auto m = m_propertyMappings.get(context);
    ErAssert(m);

    auto& mapping = m.get();

    if (id < mapping.size())
        return mapping[id];

    return nullptr;
}

bool ErebusService::propertyMappingExists(const std::string& context)
{
    auto m = m_propertyMappings.get(context);
    ErAssert(m);

    auto& mapping = m.get();
    return !mapping.empty();
}

void ErebusService::registerPropertyMapping(std::uint32_t id, const std::string& context, Er::PropertyType type, const std::string& name, const std::string& readableName)
{
    Er::Log2::debug(m_params.log, "ErebusService::registerPropertyMapping({}.{} -> {}[{}])", context, id, name, readableName);
    Er::Log2::Indent idt(m_params.log);

    auto m = m_propertyMappings.get(context);
    ErAssert(m);

    auto& mapping = m.get();
    if (id >= mapping.size())
        mapping.resize(id);

    mapping[id] = Er::allocateTransientProperty(type, name, readableName);
}

} // namespace Erp::Ipc::Grpc {}


namespace Er::Ipc::Grpc
{
    
IServer::Ptr ER_GRPC_SERVER_EXPORT create(const ServerArgs& params)
{
    return std::make_unique<Erp::Ipc::Grpc::ErebusService>(params);
}

} // namespace Er::Ipc::Grpc {}