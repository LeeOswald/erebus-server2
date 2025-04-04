#pragma once

#include <erebus/erebus.grpc.pb.h>

#include "property_mapping.hxx"
#include "protocol.hxx"
#include "session_data.hxx"
#include "trace.hxx"

#include <erebus/ipc/grpc/grpc_server.hxx>
#include <erebus/system/system/packed_time.hxx>
#include <erebus/system/property_info.hxx>
#include <erebus/system/util/exception_util.hxx>
#include <erebus/system/util/null_mutex.hxx>

#include <shared_mutex>
#include <unordered_map>

namespace Erp::Ipc::Grpc
{

class ErebusService final
    : public erebus::Erebus::CallbackService
    , public Er::Ipc::IServer
{
public:
    using ClientId = Er::Ipc::ClientId;

    ~ErebusService();
    ErebusService(const Er::Ipc::Grpc::ServerArgs& params);

    grpc::ServerUnaryReactor* GenericCall(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request, erebus::ServiceReply* reply) override;
    grpc::ServerWriteReactor<erebus::ServiceReply>* GenericStream(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request) override;

    void registerService(std::string_view request, Er::Ipc::IService::Ptr service) override;
    void unregisterService(Er::Ipc::IService* service) noexcept override;

private:
    struct SessionData
    {
        SessionData() noexcept = default;

        PropertyMapping<Er::Util::NullSharedMutex> localMapping;
        PropertyMapping<Er::Util::NullSharedMutex> remoteMapping;
    };

    using SessionCache = Erp::SessionData<ClientId, SessionData>;
    using SessionRef = SessionCache::Ref;

    class ExceptionMarshaler
        : public Er::Util::ExceptionLogger
    {
    private:
        using Base = Er::Util::ExceptionLogger;

        erebus::ServiceReply& m_response;
        SessionRef& m_session;

    public:
        ExceptionMarshaler(Er::Log2::ILogger* log, erebus::ServiceReply& response, SessionRef& session)
            : Base(log)
            , m_response(response)
            , m_session(session)
        {
        }

        Er::ResultCode operator()(const Er::Exception& e)
        {
            marshalException(&m_response, e, m_session);
            return Base::operator()(e);
        }

        Er::ResultCode operator()(const std::bad_alloc& e)
        {
            marshalException(&m_response, e);
            return Base::operator()(e);
        }

        Er::ResultCode operator()(const std::bad_cast& e)
        {
            marshalException(&m_response, e);
            return Base::operator()(e);
        }

        Er::ResultCode operator()(const std::length_error& e)
        {
            marshalException(&m_response, e);
            return Base::operator()(e);
        }

        Er::ResultCode operator()(const std::out_of_range& e)
        {
            marshalException(&m_response, e);
            return Base::operator()(e);
        }

        Er::ResultCode operator()(const std::invalid_argument& e)
        {
            marshalException(&m_response, e);
            return Base::operator()(e);
        }

        Er::ResultCode operator()(const std::exception& e)
        {
            marshalException(&m_response, e);
            return Base::operator()(e);
        }

        Er::ResultCode operator()(const std::exception_ptr& ep)
        {
            marshalException(&m_response, Er::Exception(std::source_location::current(), "Unknown exception"), m_session);
            return Base::operator()(ep);
        }
    };

    class CallReplyReactor
        : public grpc::ServerUnaryReactor
    {
    public:
        ~CallReplyReactor()
        {
            ServerTrace2(m_log, "{}.CallReplyReactor::~CallReplyReactor", Er::Format::ptr(this));
        }

        CallReplyReactor(Er::Log2::ILogger* log) noexcept
            : m_log(log)
        {
            ServerTrace2(m_log, "{}.CallReplyReactor::CallReplyReactor", Er::Format::ptr(this));
        }

    private:
        void OnDone() override 
        {
            ServerTrace2(m_log, "{}.CallReplyReactor::OnDone", Er::Format::ptr(this));

            delete this;
        }

        void OnCancel() override 
        { 
            ServerTrace2(m_log, "{}.CallReplyReactor::OnCancel", Er::Format::ptr(this));
        }

        Er::Log2::ILogger* const m_log;
    };

    class StreamReplyWriteReactor
        : public grpc::ServerWriteReactor<erebus::ServiceReply>
    {
    public:
        ~StreamReplyWriteReactor()
        {
            ServerTraceIndent2(m_log, "{}.StreamReplyWriteReactor::~StreamReplyWriteReactor", Er::Format::ptr(this));

            if (m_streamId != Er::Ipc::IService::InvalidStreamId)
            {
                if (m_service)
                    m_service->endStream(m_streamId);
            }
        }

        StreamReplyWriteReactor(Er::Log2::ILogger* log) noexcept
            : m_log(log)
            , m_streamId(Er::Ipc::IService::InvalidStreamId)
        {
            ServerTrace2(m_log, "{}.StreamReplyWriteReactor::StreamReplyWriteReactor", Er::Format::ptr(this));
        }

        void Begin(Er::Ipc::IService::Ptr service, std::string_view request, ClientId clientId, Er::PropertyBag&& args, SessionRef&& session)
        {
            ServerTraceIndent2(m_log, "{}.StreamReplyWriteReactor::Begin", Er::Format::ptr(this));

            ErAssert(!m_service);
            m_service = service;
            m_session = std::move(session);

            bool error = false;
            ExceptionMarshaler xcptHandler(m_log, m_response, session);

            try
            {
                m_streamId = service->beginStream(request, clientId, std::move(args));
            }
            catch (...)
            {
                error = true;
                Er::dispatchException(std::current_exception(), xcptHandler);
            }

            if (error)
                StartWriteAndFinish(&m_response, grpc::WriteOptions(), grpc::Status::OK); // just send the exception
            else
                Continue();
        }

        void OnWriteDone(bool ok) override 
        {
            ServerTraceIndent2(m_log, "{}.StreamReplyWriteReactor::OnWriteDone", Er::Format::ptr(this));

            if (!ok) 
                Finish(grpc::Status(grpc::StatusCode::CANCELLED, "Operation canceled"));
            else
                Continue();
        }

        void OnDone() override 
        {
            ServerTrace2(m_log, "{}.StreamReplyWriteReactor::OnDone", Er::Format::ptr(this));

            delete this;
        }

        void OnCancel() override 
        {
            ServerTrace2(m_log, "{}.StreamReplyWriteReactor::OnCancel", Er::Format::ptr(this));
        }

    private:
        void Continue()
        {
            ServerTraceIndent2(m_log, "{}.StreamReplyWriteReactor::Continue", Er::Format::ptr(this));

            m_response.Clear();

            bool error = false;
            ExceptionMarshaler xcptHandler(m_log, m_response);

            try
            {
                auto item = m_service->next(m_streamId);
                if (item.empty())
                {
                    ServerTrace2(m_log, "End of stream");
                    
                    Finish(grpc::Status::OK);
                    return;
                }
                else
                {
                    marshalReplyProps(item, &m_response);
                }
            }
            catch (...)
            {
                error = true;
                Er::dispatchException(std::current_exception(), xcptHandler);
            }

            if (error)
            {
                StartWriteAndFinish(&m_response, grpc::WriteOptions(), grpc::Status::OK); // just send the exception
            }
            else
            {
                StartWrite(&m_response);
            }
        }

        Er::Log2::ILogger* const m_log;
        Er::Ipc::IService::Ptr m_service;
        SessionRef m_session;
        Er::Ipc::IService::StreamId m_streamId = {};
        erebus::ServiceReply m_response;
    };

    Er::Ipc::IService::Ptr findService(const std::string& id) noexcept;
    Er::PropertyInfo const* unmarshalPropertyInfo(const erebus::PropertyInfo& source, SessionRef& session);
    void unmarshalPropertyInfos(const erebus::ServiceRequest* request, SessionRef& session);
    Er::PropertyBag unmarshalArgs(const erebus::ServiceRequest* request, SessionRef& session);
    static void marshalReplyProps(const Er::PropertyBag& props, erebus::ServiceReply* reply, SessionRef& session);
    static void marshalException(erebus::ServiceReply* reply, const std::exception& e);
    static void marshalException(erebus::ServiceReply* reply, const Er::Exception& e, SessionRef& session);

    static constexpr Er::PropertyInfo::Unique HighestPropertyId = 65535;

    const Er::Ipc::Grpc::ServerArgs m_params;
    Er::Log2::ILogger* const m_log;
    std::unique_ptr<grpc::Server> m_server;

    struct
    {
        std::shared_mutex lock;
        std::unordered_map<std::string, Er::Ipc::IService::Ptr> map; // uri -> service
    } m_services;

    SessionCache m_sessions;
};

} // namespace Erp::Ipc::Grpc {}