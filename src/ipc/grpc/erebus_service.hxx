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
    grpc::ServerWriteReactor<erebus::PropertyInfo>* GetPropertyMapping(grpc::CallbackServerContext* context, const erebus::Void* request) override;
    grpc::ServerReadReactor<erebus::PutPropertyMappingRequest>* PutPropertyMapping(grpc::CallbackServerContext*, ::erebus::Void* reply) override;
    grpc::ServerUnaryReactor* GenericRpc(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request, erebus::ServiceReply* reply) override;
    grpc::ServerWriteReactor<erebus::ServiceReply>* GenericStream(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request) override;

    void registerService(std::string_view request, Er::Ipc::IService::Ptr service) override;
    void unregisterService(Er::Ipc::IService* service) override;

    const Er::PropertyInfo* mapProperty(std::uint32_t id, const std::string& context) override;

    bool propertyMappingExists(const std::string& context);
    void registerPropertyMapping(std::uint32_t id, const std::string& context, Er::PropertyType type, const std::string& name, const std::string& readableName);

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
            Er::Log2::debug(m_log, "{}.ReplyUnaryReactor::~ReplyUnaryReactor", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);
        }

        ReplyUnaryReactor(Er::Log2::ILogger* log) noexcept
            : m_log(log)
        {
            Er::Log2::debug(m_log, "{}.ReplyUnaryReactor::ReplyUnaryReactor", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);
        }

    private:
        void OnDone() override 
        {
            Er::Log2::debug(m_log, "{}.ReplyUnaryReactor::OnDone", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);

            delete this;
        }

        void OnCancel() override 
        { 
            Er::Log2::debug(m_log, "{}.ReplyUnaryReactor::OnCancel", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);
        }

        Er::Log2::ILogger* const m_log;
    };

    class ReplyStreamWriteReactor
        : public grpc::ServerWriteReactor<erebus::ServiceReply>
    {
    public:
        ~ReplyStreamWriteReactor()
        {
            Er::Log2::debug(m_log, "{}.ReplyStreamWriteReactor::~ReplyStreamWriteReactor", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);

            if (m_streamId)
            {
                if (m_service)
                    m_service->endStream(m_streamId);
            }
        }

        ReplyStreamWriteReactor(Er::Log2::ILogger* log) noexcept
            : m_log(log)
        {
            Er::Log2::debug(m_log, "{}.ReplyStreamWriteReactor::ReplyStreamWriteReactor", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);
        }

        void Begin(Er::Ipc::IService::Ptr service, std::string_view request, std::string_view cookie, const Er::PropertyBag& args)
        {
            Er::Log2::debug(m_log, "{}.ReplyStreamWriteReactor::Begin", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);

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
                StartWriteAndFinish(&m_response, grpc::WriteOptions(), grpc::Status::OK); // just send the exception
            }
            else
            {
                Continue();
            }
        }

        void OnWriteDone(bool ok) override 
        {
            Er::Log2::debug(m_log, "{}.ReplyStreamWriteReactor::OnWriteDone", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);

            if (!ok) 
                Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Unexpected Failure"));
            else
                Continue();
        }

        void OnDone() override 
        {
            Er::Log2::debug(m_log, "{}.ReplyStreamWriteReactor::OnDone", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);

            delete this;
        }

        void OnCancel() override 
        {
            Er::Log2::debug(m_log, "{}.ReplyStreamWriteReactor::OnCancel", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);
        }

    private:
        void Continue()
        {
            Er::Log2::debug(m_log, "{}.ReplyStreamWriteReactor::Continue", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);

            m_response.Clear();
            bool error = false;
            ExceptionMarshaler xcptHandler(m_log, m_response);

            try
            {
                auto item = m_service->next(m_streamId);
                if (item.empty())
                {
                    // end of stream
                    Finish(grpc::Status::OK);
                    return;
                }
                else
                {
                    m_response.set_result(erebus::SUCCESS);
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
        Er::Ipc::IService::Ptr m_service;
        Er::Ipc::IService::StreamId m_streamId = {};
        erebus::ServiceReply m_response;
    };

    class PropertyInfoStreamWriteReactor
        : public grpc::ServerWriteReactor<erebus::PropertyInfo>
    {
    public:
        ~PropertyInfoStreamWriteReactor()
        {
            Er::Log2::debug(m_log, "{}.PropertyInfoStreamWriteReactor::~PropertyInfoStreamWriteReactor", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);
        }

        PropertyInfoStreamWriteReactor(Er::Log2::ILogger* log) noexcept
            : m_log(log)
        {
            Er::Log2::debug(m_log, "{}.PropertyInfoStreamWriteReactor::PropertyInfoStreamWriteReactor", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);
        }

        void Begin()
        {
            Er::Log2::debug(m_log, "{}.PropertyInfoStreamWriteReactor::Begin", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);

            ErAssert(m_properties.empty());
            ErAssert(m_next == 0);

            Er::enumerateProperties([this](const Er::PropertyInfo* pi) -> bool
            {
                m_properties.push_back(pi);
                return true;
            });

            Continue();
        }

        void OnWriteDone(bool ok) override
        {
            Er::Log2::debug(m_log, "{}.PropertyInfoStreamWriteReactor::OnWriteDone", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);

            if (!ok)
                Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Unexpected Failure"));
            else
                Continue();
        }

        void OnDone() override
        {
            Er::Log2::debug(m_log, "{}.PropertyInfoStreamWriteReactor::OnDone", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);

            delete this;
        }

        void OnCancel() override
        {
            Er::Log2::debug(m_log, "{}.PropertyInfoStreamWriteReactor::OnCancel", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);
        }

    private:
        void Continue()
        {
            Er::Log2::debug(m_log, "{}.PropertyInfoStreamWriteReactor::Continue", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);

            m_response.Clear();
            
            auto pi = m_properties[m_next++];
            m_response.set_id(pi->unique());
            m_response.set_type(static_cast<std::uint32_t>(pi->type()));
            m_response.set_name(pi->name());
            m_response.set_readable_name(pi->readableName());

            StartWrite(&m_response);
        }

        Er::Log2::ILogger* const m_log;
        std::vector<const Er::PropertyInfo*> m_properties;
        std::size_t m_next = 0;
        erebus::PropertyInfo m_response;
    };

    class PutPropertyMappingStreamReadReactor
        : public grpc::ServerReadReactor<erebus::PutPropertyMappingRequest>
    {
    public:
        ~PutPropertyMappingStreamReadReactor()
        {
            Er::Log2::debug(m_log, "{}.PutPropertyMappingStreamReadReactor::~PutPropertyMappingStreamReadReactor", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);
        }

        PutPropertyMappingStreamReadReactor(Er::Log2::ILogger* log, ErebusService* owner)
            : m_log(log)
            , m_owner(owner)
        {
            Er::Log2::debug(m_log, "{}.PutPropertyMappingStreamReadReactor::PutPropertyMappingStreamReadReactor", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);

            Continue();
        }

        void OnReadDone(bool ok) override
        {
            Er::Log2::debug(m_log, "{}.PutPropertyMappingStreamReadReactor::OnReadDone", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);

            if (!ok)
            {
                Finish(grpc::Status(grpc::StatusCode::UNKNOWN, "Unexpected failure"));
            }
            else
            {
                auto& cookie = m_request.cookie();
                auto& rawInfo = m_request.mapping();

                auto type = static_cast<Er::PropertyType>(rawInfo.type());
                auto id = rawInfo.id();
                auto& name = rawInfo.name();
                auto& readableName = rawInfo.readable_name();

                m_owner->registerPropertyMapping(id, cookie, type, name, readableName);

                Continue();
            }
        }

    private:
        void Continue()
        {
            Er::Log2::debug(m_log, "{}.PutPropertyMappingStreamReadReactor::Continue", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);

            m_request.Clear();
            StartRead(&m_request);
        }

        void OnDone() override
        {
            Er::Log2::debug(m_log, "{}.PutPropertyMappingStreamReadReactor::OnDone", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);

            delete this;
        }

        void OnCancel() override
        {
            Er::Log2::debug(m_log, "{}.PutPropertyMappingStreamReadReactor::OnCancel", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);
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
    std::unique_ptr<grpc::Server> m_server;

    struct
    {
        mutable std::shared_mutex lock;
        std::unordered_map<std::string, Er::Ipc::IService::Ptr> map; // uri -> service
    } m_services;

    Erp::SessionData<std::string, std::vector<const Er::PropertyInfo*>> m_propertyMappings;
};

} // namespace Erp::Ipc::Grpc {}