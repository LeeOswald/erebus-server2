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
    , m_sessions(std::chrono::seconds(600)) // 10 mins
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
    Er::Log2::debug(m_params.log, "{}.ErebusService::GenericRpc", Er::Format::ptr(this));
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
    [[maybe_unused]] auto session = m_sessions.get(cookie);

    reactor->Finish(grpc::Status::OK);
    return reactor.release();
}

grpc::ServerUnaryReactor* ErebusService::GenericCall(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request, erebus::ServiceReply* reply)
{
    Er::Log2::debug(m_params.log, "{}.ErebusService::GenericCall", Er::Format::ptr(this));
    Er::Log2::Indent idt(m_params.log);

    auto reactor = std::make_unique<ReplyUnaryReactor>(m_params.log);
    if (context->IsCancelled()) [[unlikely]]
    {
        Er::Log2::warning(m_params.log, "Request cancelled");
        return reactor.release();
    }

    auto& requestStr = request->request();
    
    auto service = findService(requestStr);
    if (!service) [[unlikely]]
    {
        auto msg = Er::format("No handlers for [{}]", requestStr);
        Er::Log2::writeln(m_params.log, Er::Log2::Level::Error, msg);
        reactor->Finish(grpc::Status(grpc::UNIMPLEMENTED, msg));
        return reactor.release();
    }

    reply->set_mappingver(Erp::propertyMappingVersion());

    static std::string empty;
    const std::string& cookie = request->has_cookie() ? request->cookie() : empty;

    Er::Log2::info(m_params.log, "Req [{}] from {}:{}", requestStr, context->peer(), cookie);

    auto mappingVer = request->mappingver();

    ExceptionMarshaler xcptHandler(m_params.log, *reply);
    try
    {
        auto valid = propertyMappingValid(cookie, mappingVer);
        if ((request->args_size() > 0) && (cookie.empty() || !valid.first))
        {
            Er::Log2::debug(Er::Log2::get(), "Property mapping expired: remote v.{} local v.{}", mappingVer, valid.second);
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
    Er::Log2::debug(m_params.log, "{}.ErebusService::GenericStream", Er::Format::ptr(this));
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
    
    auto mappingVer = request->mappingver();

    std::string errorMsg;
    Er::Util::ExceptionLogger xcptLogger(m_params.log);
    try
    {
        auto valid = propertyMappingValid(cookie, mappingVer);
        if ((request->args_size() > 0) && (cookie.empty() || !valid.first))
        {
            Er::Log2::debug(Er::Log2::get(), "Property mapping expired: remote v.{} vs local v.{}", mappingVer, valid.second);
            reactor->SendPropertyMappingExpired();
        }
        else
        {
            auto args = unmarshalArgs(request, cookie);
            reactor->Begin(service, requestStr, cookie, args);
        }
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

grpc::ServerWriteReactor<erebus::GetPropertyMappingReply>* ErebusService::GetPropertyMapping(grpc::CallbackServerContext* context, const erebus::Void* request)
{
    Er::Log2::debug(m_params.log, "{}.ErebusService::GetPropertyMapping", Er::Format::ptr(this));
    Er::Log2::Indent idt(m_params.log);

    Er::Log2::info(m_params.log, "Request from {}", context->peer());

    auto reactor = std::make_unique<PropertyInfoStreamWriteReactor>(m_params.log);
    reactor->Begin();
    
    return reactor.release();
}

grpc::ServerReadReactor<erebus::PutPropertyMappingRequest>* ErebusService::PutPropertyMapping(grpc::CallbackServerContext* context, ::erebus::Void* response)
{
    Er::Log2::debug(m_params.log, "{}.ErebusService::PutPropertyMapping", Er::Format::ptr(this));
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
    auto session = m_sessions.get(context);
    ErAssert(session);

    auto& mapping = session.get();

    if (id < mapping.propertyMapping.size())
        return mapping.propertyMapping[id];

    return nullptr;
}

std::pair<bool, std::uint32_t> ErebusService::propertyMappingValid(const std::string& context, std::uint32_t mappingVer)
{
    auto session = m_sessions.get(context);
    ErAssert(session);

    auto& mapping = session.get();
    if (mapping.propertyMapping.empty())
        return std::make_pair(false, std::uint32_t(-1));

    return std::make_pair((mapping.mappingVersion == mappingVer), mapping.mappingVersion);
}

void ErebusService::registerPropertyMapping(std::uint32_t version, std::uint32_t id, const std::string& context, Er::PropertyType type, const std::string& name, const std::string& readableName)
{
    Er::Log2::debug(m_params.log, "{}.ErebusService::registerPropertyMapping(v.{} {}.{} -> {}[{}])", Er::Format::ptr(this), version, context, id, name, readableName);
    Er::Log2::Indent idt(m_params.log);

    auto session = m_sessions.get(context);
    ErAssert(session);

    auto& mapping = session.get();
    mapping.mappingVersion = version;
    auto& m = mapping.propertyMapping;
    if (id >= m.size())
        m.resize(id + 1);

    m[id] = Erp::allocateTransientProperty(type, name, readableName);
}

} // namespace Erp::Ipc::Grpc {}


namespace Er::Ipc::Grpc
{
    
IServer::Ptr ER_GRPC_SERVER_EXPORT create(const ServerArgs& params)
{
    return std::make_unique<Erp::Ipc::Grpc::ErebusService>(params);
}

} // namespace Er::Ipc::Grpc {}