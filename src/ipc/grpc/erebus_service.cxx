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
    , m_log(params.log.get())
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

Er::PropertyBag ErebusService::unmarshalArgs(const erebus::ServiceRequest* request, std::uint32_t clientId)
{
    Er::PropertyBag bag;

    int count = request->args_size();
    if (count > 0)
    {
        bag.reserve(count);
        
        for (int i = 0; i < count; ++i)
        {
            auto& arg = request->args(i);
            bag.push_back(Erp::Protocol::getProperty(arg, this, clientId));
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
    ServerTraceIndent2(m_log, "{}.ErebusService::Ping", Er::Format::ptr(this));

    auto reactor = std::make_unique<ReplyUnaryReactor>(m_log);
    if (context->IsCancelled()) [[unlikely]]
    {
        ErLogWarning2(m_log, "Request cancelled");
        return reactor.release();
    }

    auto timestamp = request->timestamp();
    reply->set_timestamp(timestamp);

    auto& payload = request->payload();
    reply->set_payload(payload);
    
    auto clientId = request->clientid();
    ErLogInfo2(m_log, "Ping with {} bytes of data from {}:{}", payload.length(), context->peer(), clientId);

    // touch session data
    [[maybe_unused]] auto session = m_sessions.get(clientId);

    reactor->Finish(grpc::Status::OK);
    return reactor.release();
}

grpc::ServerUnaryReactor* ErebusService::GenericCall(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request, erebus::ServiceReply* reply)
{
    ServerTraceIndent2(m_log, "{}.ErebusService::GenericCall", Er::Format::ptr(this));

    auto reactor = std::make_unique<ReplyUnaryReactor>(m_log);
    if (context->IsCancelled()) [[unlikely]]
    {
        ErLogWarning2(m_log, "Request cancelled");
        return reactor.release();
    }

    auto& requestStr = request->request();
    
    auto service = findService(requestStr);
    if (!service) [[unlikely]]
    {
        auto msg = Er::format("No handlers for [{}]", requestStr);
        Er::Log2::writeln(m_log, Er::Log2::Level::Error, msg);
        reactor->Finish(grpc::Status(grpc::UNIMPLEMENTED, msg));
        return reactor.release();
    }

    reply->set_mappingver(Erp::propertyMappingVersion());

    std::uint32_t clientId = request->has_clientid() ? request->clientid() : std::uint32_t(-1);

    ErLogInfo2(m_log, "Req [{}] from {}:{}", requestStr, context->peer(), clientId);

    auto mappingVer = request->mappingver();

    ExceptionMarshaler xcptHandler(m_log, *reply);
    try
    {
        auto valid = propertyMappingValid(clientId, mappingVer);
        if ((request->args_size() > 0) && ((clientId == std::uint32_t(-1)) || !valid.first))
        {
            ErLogDebug2(m_log, "Property mapping expired: remote v.{} local v.{}", mappingVer, valid.second);
            reply->set_result(erebus::CallResult::PROPERTY_MAPPING_EXPIRED);
            
        }
        else
        {
            auto args = unmarshalArgs(request, clientId);
            auto result = service->request(requestStr, clientId, args);
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
    ServerTraceIndent2(m_log, "{}.ErebusService::GenericStream", Er::Format::ptr(this));

    auto reactor = std::make_unique<ReplyStreamWriteReactor>(m_log);

    auto& requestStr = request->request();
    ErLogInfo2(m_log, "Strm [{}] to {}", requestStr, context->peer());

    auto service = findService(requestStr);
    if (!service)
    {
        auto msg = Er::format("No handlers for [{}]", requestStr);
        Er::Log2::writeln(m_log, Er::Log2::Level::Error, msg);
        reactor->Finish(grpc::Status(grpc::UNIMPLEMENTED, msg));
        return reactor.release();
    }

    std::uint32_t clientId = request->has_clientid() ? request->clientid() : std::uint32_t(-1);
    
    auto mappingVer = request->mappingver();

    std::string errorMsg;
    Er::Util::ExceptionLogger xcptLogger(m_log);
    try
    {
        auto valid = propertyMappingValid(clientId, mappingVer);
        if ((request->args_size() > 0) && ((clientId == std::uint32_t(-1)) || !valid.first))
        {
            ErLogDebug2(m_log, "Property mapping expired: remote v.{} vs local v.{}", mappingVer, valid.second);
            reactor->SendPropertyMappingExpired();
        }
        else
        {
            auto args = unmarshalArgs(request, clientId);
            reactor->Begin(service, requestStr, clientId, args);
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
    ServerTraceIndent2(m_log, "{}.ErebusService::GetPropertyMapping(peer={})", Er::Format::ptr(this), context->peer());

    auto reactor = std::make_unique<PropertyInfoStreamWriteReactor>(m_log);
    reactor->Begin();
    
    return reactor.release();
}

grpc::ServerReadReactor<erebus::PutPropertyMappingRequest>* ErebusService::PutPropertyMapping(grpc::CallbackServerContext* context, ::erebus::Void* response)
{
    ServerTraceIndent2(m_log, "{}.ErebusService::PutPropertyMapping(peer={})", Er::Format::ptr(this), context->peer());

    auto reactor = std::make_unique<PutPropertyMappingStreamReadReactor>(m_log, this);
    return reactor.release();
}

void ErebusService::registerService(std::string_view request, Er::Ipc::IService::Ptr service)
{
    std::lock_guard l(m_services.lock);

    std::string id(request);
    auto it = m_services.map.find(id);
    if (it != m_services.map.end())
        ErThrow(Er::format("Service for [{}] is already registered", id));

    ErLogInfo2(m_log, "Registered service {} for [{}]", Er::Format::ptr(service.get()), id);

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
            ErLogInfo2(m_log, "Unregistered service {}", Er::Format::ptr(service));

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
        ErLogError2(m_log, "Service {} is not registered", Er::Format::ptr(service));
}

const Er::PropertyInfo* ErebusService::mapProperty(std::uint32_t id, std::uint32_t clientId)
{
    auto session = m_sessions.get(clientId);
    ErAssert(session);

    auto& mapping = session.get();

    if (id < mapping.propertyMapping.size())
        return mapping.propertyMapping[id];

    return nullptr;
}

std::pair<bool, std::uint32_t> ErebusService::propertyMappingValid(std::uint32_t clientId, std::uint32_t mappingVer)
{
    auto session = m_sessions.get(clientId);
    ErAssert(session);

    auto& mapping = session.get();
    if (mapping.propertyMapping.empty())
        return std::make_pair(false, std::uint32_t(-1));

    return std::make_pair((mapping.mappingVersion == mappingVer), mapping.mappingVersion);
}

void ErebusService::registerPropertyMapping(std::uint32_t version, std::uint32_t id, std::uint32_t clientId, Er::PropertyType type, const std::string& name, const std::string& readableName)
{
    ServerTraceIndent2(m_log, "{}.ErebusService::registerPropertyMapping(v.{} {}.{} -> {}[{}])", Er::Format::ptr(this), version, clientId, id, name, readableName);

    auto session = m_sessions.get(clientId);
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