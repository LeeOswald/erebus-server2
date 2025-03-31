#pragma once

#include "protocol.hxx"
#include "session_data.hxx"

#include <erebus/erebus.grpc.pb.h>

#include <erebus/ipc/grpc/grpc_server.hxx>
#include <erebus/system/property_info.hxx>
#include <erebus/system/util/exception_util.hxx>

#include <atomic>
#include <shared_mutex>
#include <unordered_map>

namespace Erp::Ipc::Grpc
{

class ErebusService final
    : public erebus::Erebus::CallbackService
    , public Er::Ipc::IServer
    , public Er::IPropertyMapping
{
public:
    ~ErebusService();
    ErebusService(const Er::Ipc::Grpc::ServerArgs& params);

    grpc::ServerUnaryReactor* Ping(grpc::CallbackServerContext* context, const erebus::PingRequest* request, erebus::PingReply* reply) override;
    grpc::ServerWriteReactor<erebus::GetPropertyMappingReply>* GetPropertyMapping(grpc::CallbackServerContext* context, const erebus::Void* request) override;
    grpc::ServerReadReactor<erebus::PutPropertyMappingRequest>* PutPropertyMapping(grpc::CallbackServerContext*, ::erebus::Void* reply) override;
    grpc::ServerUnaryReactor* GenericCall(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request, erebus::ServiceReply* reply) override;
    grpc::ServerWriteReactor<erebus::ServiceReply>* GenericStream(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request) override;

    void registerService(std::string_view request, Er::Ipc::IService::Ptr service) override;
    void unregisterService(Er::Ipc::IService* service) override;

    const Er::PropertyInfo* mapProperty(std::uint32_t id, const std::string& context) override;

    std::pair<bool, std::uint32_t> propertyMappingValid(const std::string& context, std::uint32_t mappingVer);
    void registerPropertyMapping(std::uint32_t version, std::uint32_t id, const std::string& context, Er::PropertyType type, const std::string& name, const std::string& readableName);

private:
    class ExceptionMarshaler
        : public Er::Util::ExceptionLogger
    {
    private:
        using Base = Er::Util::ExceptionLogger;

        erebus::ServiceReply& m_response;

    public:
        ExceptionMarshaler(Er::Log2::ILogger* log, erebus::ServiceReply& response)
            : Base(log)
            , m_response(response)
        {
        }

        Er::ResultCode operator()(const Er::Exception& e)
        {
            marshalException(&m_response, e);
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
            marshalException(&m_response, Er::Exception(std::source_location::current(), "Unknown exception"));
            return Base::operator()(ep);
        }
    };

    class ReplyUnaryReactor
        : public grpc::ServerUnaryReactor
    {
    public:
        ~ReplyUnaryReactor()
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.ReplyUnaryReactor::~ReplyUnaryReactor", Er::Format::ptr(this));
        }

        ReplyUnaryReactor(Er::Log2::ILogger* log) noexcept
            : m_log(log)
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.ReplyUnaryReactor::ReplyUnaryReactor", Er::Format::ptr(this));
        }

    private:
        void OnDone() override 
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.ReplyUnaryReactor::OnDone", Er::Format::ptr(this));

            delete this;
        }

        void OnCancel() override 
        { 
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.ReplyUnaryReactor::OnCancel", Er::Format::ptr(this));
        }

        Er::Log2::ILogger* const m_log;
    };

    class ReplyStreamWriteReactor
        : public grpc::ServerWriteReactor<erebus::ServiceReply>
    {
    public:
        ~ReplyStreamWriteReactor()
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.ReplyStreamWriteReactor::~ReplyStreamWriteReactor", Er::Format::ptr(this));

            if (m_streamId)
            {
                if (m_service)
                    m_service->endStream(m_streamId);
            }
        }

        ReplyStreamWriteReactor(Er::Log2::ILogger* log) noexcept
            : m_log(log)
            , m_mappingVersion(Erp::propertyMappingVersion())
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.ReplyStreamWriteReactor::ReplyStreamWriteReactor", Er::Format::ptr(this));
        }

        void SendPropertyMappingExpired()
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.ReplyStreamWriteReactor::SendPropertyMappingExpired", Er::Format::ptr(this));

            m_response.set_result(erebus::PROPERTY_MAPPING_EXPIRED);
            m_response.set_mappingver(m_mappingVersion);
            StartWriteAndFinish(&m_response, grpc::WriteOptions(), grpc::Status::OK);
        }

        void Begin(Er::Ipc::IService::Ptr service, std::string_view request, std::string_view cookie, const Er::PropertyBag& args)
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.ReplyStreamWriteReactor::Begin", Er::Format::ptr(this));

            ErAssert(!m_service);
            m_service = service;

            bool error = false;
            ExceptionMarshaler xcptHandler(m_log, m_response);

            try
            {
                m_streamId = service->beginStream(request, cookie, args);
            }
            catch (...)
            {
                error = true;
                Er::dispatchException(std::current_exception(), xcptHandler);
            }

            if (error)
            {
                m_response.set_result(erebus::FAILURE);
                m_response.set_mappingver(m_mappingVersion);
                StartWriteAndFinish(&m_response, grpc::WriteOptions(), grpc::Status::OK); // just send the exception
            }
            else
            {
                Continue();
            }
        }

        void OnWriteDone(bool ok) override 
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.ReplyStreamWriteReactor::OnWriteDone", Er::Format::ptr(this));

            if (!ok) 
                Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Unexpected Failure"));
            else
                Continue();
        }

        void OnDone() override 
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.ReplyStreamWriteReactor::OnDone", Er::Format::ptr(this));

            delete this;
        }

        void OnCancel() override 
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.ReplyStreamWriteReactor::OnCancel", Er::Format::ptr(this));
        }

    private:
        void Continue()
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.ReplyStreamWriteReactor::Continue", Er::Format::ptr(this));

            m_response.Clear();
            bool error = false;
            ExceptionMarshaler xcptHandler(m_log, m_response);

            try
            {
                auto item = m_service->next(m_streamId);
                if (item.empty())
                {
                    ErLogDebug2(m_log, "End of stream");
                    // end of stream
                    Finish(grpc::Status::OK);
                    return;
                }
                else
                {
                    m_response.set_result(erebus::SUCCESS);
                    m_response.set_mappingver(m_mappingVersion);
                    marshalReplyProps(item, &m_response);
                }
            }
            catch (...)
            {
                error = true;
                m_response.set_result(erebus::FAILURE);
                Er::dispatchException(std::current_exception(), xcptHandler);
            }

            if (error)
                StartWriteAndFinish(&m_response, grpc::WriteOptions(), grpc::Status::OK); // just send the exception
            else
                StartWrite(&m_response);
        }

        Er::Log2::ILogger* const m_log;
        std::uint32_t m_mappingVersion;
        Er::Ipc::IService::Ptr m_service;
        Er::Ipc::IService::StreamId m_streamId = {};
        erebus::ServiceReply m_response;
    };

    class PropertyInfoStreamWriteReactor
        : public grpc::ServerWriteReactor<erebus::GetPropertyMappingReply>
    {
    public:
        ~PropertyInfoStreamWriteReactor()
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.PropertyInfoStreamWriteReactor::~PropertyInfoStreamWriteReactor", Er::Format::ptr(this));
        }

        PropertyInfoStreamWriteReactor(Er::Log2::ILogger* log) noexcept
            : m_log(log)
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.PropertyInfoStreamWriteReactor::PropertyInfoStreamWriteReactor", Er::Format::ptr(this));
        }

        void Begin()
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.PropertyInfoStreamWriteReactor::Begin", Er::Format::ptr(this));

            ErAssert(m_properties.empty());
            ErAssert(m_next == 0);

            m_version = Er::enumerateProperties([this](const Er::PropertyInfo* pi) -> bool
            {
                m_properties.push_back(pi);
                return true;
            });

            ErLogDebug2(m_log, "Found {} local properties ver {}", Er::Format::ptr(this), m_properties.size(), m_version);

            Continue();
        }

        void OnWriteDone(bool ok) override
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.PropertyInfoStreamWriteReactor::OnWriteDone", Er::Format::ptr(this));

            if (!ok)
                Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Unexpected Failure"));
            else
                Continue();
        }

        void OnDone() override
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.PropertyInfoStreamWriteReactor::OnDone", Er::Format::ptr(this));

            delete this;
        }

        void OnCancel() override
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.PropertyInfoStreamWriteReactor::OnCancel", Er::Format::ptr(this));
        }

    private:
        void Continue()
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.PropertyInfoStreamWriteReactor::Continue", Er::Format::ptr(this));

            if (m_next < m_properties.size())
            {
                m_response.Clear();

                auto pi = m_properties[m_next++];
                auto m = m_response.mutable_mapping();
                m->set_id(pi->unique());
                m->set_type(static_cast<std::uint32_t>(pi->type()));
                m->set_name(pi->name());
                m->set_readable_name(pi->readableName());

                m_response.set_mappingver(m_version);

                StartWrite(&m_response);
            }
            else
            {
                Finish(grpc::Status::OK);
            }
        }

        Er::Log2::ILogger* const m_log;
        std::uint32_t m_version = std::uint32_t(-1);
        std::vector<const Er::PropertyInfo*> m_properties;
        std::size_t m_next = 0;
        erebus::GetPropertyMappingReply m_response;
    };

    class PutPropertyMappingStreamReadReactor
        : public grpc::ServerReadReactor<erebus::PutPropertyMappingRequest>
    {
    public:
        ~PutPropertyMappingStreamReadReactor()
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.PutPropertyMappingStreamReadReactor::~PutPropertyMappingStreamReadReactor", Er::Format::ptr(this));
        }

        PutPropertyMappingStreamReadReactor(Er::Log2::ILogger* log, ErebusService* owner)
            : m_log(log)
            , m_owner(owner)
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.PutPropertyMappingStreamReadReactor::PutPropertyMappingStreamReadReactor", Er::Format::ptr(this));

            Continue();
        }

    private:
        void OnReadDone(bool ok) override
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.PutPropertyMappingStreamReadReactor::OnReadDone({})", Er::Format::ptr(this), ok);

            if (!ok)
            {
                Finish(grpc::Status::OK);
            }
            else
            {
                auto& cookie = m_request.cookie();
                auto& rawInfo = m_request.mapping();

                auto type = static_cast<Er::PropertyType>(rawInfo.type());
                auto id = rawInfo.id();
                auto& name = rawInfo.name();
                auto& readableName = rawInfo.readable_name();

                auto version = m_request.mappingver();

                m_owner->registerPropertyMapping(version, id, cookie, type, name, readableName);

                Continue();
            }
        }

        void Continue()
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.PutPropertyMappingStreamReadReactor::Continue", Er::Format::ptr(this));

            m_request.Clear();
            StartRead(&m_request);
        }

        void OnDone() override
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.PutPropertyMappingStreamReadReactor::OnDone", Er::Format::ptr(this));

            delete this;
        }

        void OnCancel() override
        {
            ErLogIndent2(m_log, Er::Log2::Level::Debug, "{}.PutPropertyMappingStreamReadReactor::OnCancel", Er::Format::ptr(this));
        }

        Er::Log2::ILogger* const m_log;
        ErebusService* const m_owner;
        erebus::PutPropertyMappingRequest m_request;
    };

    Er::Ipc::IService::Ptr findService(const std::string& id) const;
    Er::PropertyBag unmarshalArgs(const erebus::ServiceRequest* request, const std::string& context);
    static void marshalReplyProps(const Er::PropertyBag& props, erebus::ServiceReply* reply);
    static void marshalException(erebus::ServiceReply* reply, const std::exception& e);
    static void marshalException(erebus::ServiceReply* reply, const Er::Exception& e);

    const Er::Ipc::Grpc::ServerArgs m_params;
    Er::Log2::ILogger* const m_log;
    std::unique_ptr<grpc::Server> m_server;

    struct
    {
        mutable std::shared_mutex lock;
        std::unordered_map<std::string, Er::Ipc::IService::Ptr> map; // uri -> service
    } m_services;

    struct SessionData
    {
        std::vector<const Er::PropertyInfo*> propertyMapping;
        std::uint32_t mappingVersion = std::uint32_t(-1);
    };

    Erp::SessionData<std::string, SessionData> m_sessions;
};

} // namespace Erp::Ipc::Grpc {}