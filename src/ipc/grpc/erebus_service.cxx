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

Er::Ipc::IService::Ptr ErebusService::findService(const std::string& id) noexcept
{
    std::shared_lock l(m_services.lock);

    auto it = m_services.map.find(id);
    if (it != m_services.map.end())
    {
        return it->second;
    }

    return {};
}

Er::PropertyInfo const* ErebusService::unmarshalPropertyInfo(const erebus::PropertyInfo& source, SessionRef& session)
{
    auto& remoteProps = session.get().remoteMapping;

    auto id = source.id();
    if (id > HighestPropertyId)
        ErThrow(Er::format("Property ID {} exceeds limit", id));

    auto info = remoteProps.lookup(id);
    if (!info)
    {
        auto type = static_cast<Er::PropertyType>(source.type());
        auto& name = source.name();
        auto& readableName = source.readablename();

        info = Erp::allocateTransientProperty(type, name, readableName);

        remoteProps.add(id, info);
    }

    return info;
}

void ErebusService::unmarshalPropertyInfos(const erebus::ServiceRequest* request, SessionRef& session)
{
    auto count = request->mapping_size();
    if (!count)
        return;

    for (decltype(count) i = 0; i < count; ++i)
    {
        auto& pi = request->mapping(i);

        unmarshalPropertyInfo(pi, session);
    }
}

Er::PropertyBag ErebusService::unmarshalArgs(const erebus::ServiceRequest* request, SessionRef& session)
{
    Er::PropertyBag bag;

    int count = request->args_size();
    if (count > 0)
    {
        bag.reserve(count);

        auto& remoteProps = session.get().remoteMapping;
                
        for (int i = 0; i < count; ++i)
        {
            auto& arg = request->args(i);
            auto info = remoteProps.lookup(arg.id());
            if (!info)
                ErThrow(Er::format("Unknown property id {} of type {} in client request", arg.id(), Erp::Protocol::propTypeToString(arg)));

            bag.push_back(Erp::Protocol::getProperty(arg, info));
        }
    }

    return bag;
}

void ErebusService::marshalReplyProps(const Er::PropertyBag& props, erebus::ServiceReply* reply, SessionRef& session)
{
    if (props.empty())
        return;

    auto& localProps = session.get().localMapping;

    auto out = reply->mutable_props();
    for (auto& prop : props)
    {
        // send PropertyInfo if necessary
        auto id = prop.unique();
        auto info = prop.info();
        if (localProps.addIfMissing(id, info))
        {
            auto m = reply->add_mapping();
            Erp::Protocol::assignPropertyInfo(*m, info);
        }

        // send actual property
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

void ErebusService::marshalException(erebus::ServiceReply* reply, const Er::Exception& e, SessionRef& session)
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
        auto& localProps = session.get().localMapping;

        auto mutableProps = exception->mutable_props();
        mutableProps->Reserve(properties.size());

        for (auto& property : properties)
        {
            // send PropertyInfo if necessary
            auto id = property.unique();
            auto info = property.info();
            if (localProps.addIfMissing(id, info))
            {
                auto m = reply->add_mapping();
                Erp::Protocol::assignPropertyInfo(*m, info);
            }

            auto mutableProp = mutableProps->Add();
            Erp ::Protocol::assignProperty(*mutableProp, property);
        }
    }
}

grpc::ServerUnaryReactor* ErebusService::GenericCall(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request, erebus::ServiceReply* reply)
{
    ServerTraceIndent2(m_log, "{}.ErebusService::GenericCall", Er::Format::ptr(this));

    std::optional<Er::System::PackedTime::ValueType> timestamp;
    if (request->has_timestamp())
        timestamp = request->timestamp();

    auto timeStarted = Er::System::PackedTime::now();

    auto reactor = std::make_unique<CallReplyReactor>(m_log);
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

    ClientId clientId = request->clientid();
    ErLogInfo2(m_log, "Req [{}] from {}:{}", requestStr, context->peer(), clientId);

    auto session = m_sessions.get(clientId);
    if (!session)
    {
        Er::Log2::writeln(m_log, Er::Log2::Level::Error, "No session found for call");
        reactor->Finish(grpc::Status(grpc::UNAUTHENTICATED, "No session found for call"));
        return reactor.release();
    }

    ExceptionMarshaler xcptHandler(m_log, *reply, session);
    try
    {
        unmarshalPropertyInfos(request, session);

        auto args = unmarshalArgs(request, session);
        auto result = service->request(requestStr, clientId, std::move(args));
        marshalReplyProps(result, reply, session);

        if (timestamp)
            reply->set_timestamp(*timestamp);

        auto timeFinished = Er::System::PackedTime::now();
        reply->set_duration(timeFinished - timeStarted);
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
    ServerTraceIndent2(m_log, "{}.ErebusService::GenericStream", Er::Format::ptr(this));

    auto reactor = std::make_unique<StreamReplyWriteReactor>(m_log);

    auto& requestStr = request->request();
    
    auto service = findService(requestStr);
    if (!service)
    {
        auto msg = Er::format("No handlers for [{}]", requestStr);
        Er::Log2::writeln(m_log, Er::Log2::Level::Error, msg);
        reactor->Finish(grpc::Status(grpc::UNIMPLEMENTED, msg));
        return reactor.release();
    }

    ClientId clientId = request->clientid();
    ErLogInfo2(m_log, "Strm [{}] to {}:{}", requestStr, context->peer(), clientId);

    auto session = m_sessions.get(clientId);
    if (!session)
    {
        Er::Log2::writeln(m_log, Er::Log2::Level::Error, "No session found for call");
        reactor->Finish(grpc::Status(grpc::UNAUTHENTICATED, "No session found for call"));
        return reactor.release();
    }

    std::string errorMsg;
    Er::Util::ExceptionLogger xcptLogger(m_log);
    try
    {
        unmarshalPropertyInfos(request, session);

        auto args = unmarshalArgs(request, session);
        reactor->Begin(service, requestStr, clientId, std::move(args), std::move(session));
        
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


} // namespace Erp::Ipc::Grpc {}


namespace Er::Ipc::Grpc
{
    
IServer::Ptr ER_GRPC_SERVER_EXPORT create(const ServerArgs& params)
{
    return std::make_unique<Erp::Ipc::Grpc::ErebusService>(params);
}

} // namespace Er::Ipc::Grpc {}