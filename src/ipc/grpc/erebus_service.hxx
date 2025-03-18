#pragma once

#include "protocol.hxx"

#include <erebus/erebus.grpc.pb.h>

#include <erebus/ipc/grpc/grpc_server.hxx>
#include <erebus/system/util/exception_util.hxx>

#include <atomic>
#include <shared_mutex>
#include <unordered_map>

namespace Erp::Ipc::Grpc
{

class ErebusCbService final
    : public erebus::Erebus::CallbackService
    , public Er::Ipc::IServer
{
public:
    ~ErebusCbService();
    ErebusCbService(const Er::Ipc::Grpc::ServerArgs& params);

    grpc::ServerUnaryReactor* GenericRpc(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request, erebus::ServiceReply* reply) override;
    grpc::ServerWriteReactor<erebus::ServiceReply>* GenericStream(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request) override;

    void registerService(std::string_view request, Er::Ipc::IService::Ptr service) override;
    void unregisterService(Er::Ipc::IService* service) override;

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

        Er::Log2::ILogger* m_log;
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
                StartWriteAndFinish(&m_response, grpc::WriteOptions(), grpc::Status::OK); // just send the exception
            else
                Continue();
        }

        void OnWriteDone(bool ok) override 
        {
            Er::Log2::debug(m_log, "{}.ReplyStreamWriteReactor::OnWriteDone", Er::Format::ptr(this));
            Er::Log2::Indent idt(m_log);

            if (!ok) 
                Finish(grpc::Status(grpc::StatusCode::UNKNOWN, "Unexpected Failure"));
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
                    marshalReplyProps(item, &m_response);
                }
            }
            catch (...)
            {
                error = true;
                Er::dispatchException(std::current_exception(), xcptHandler);
            }

            if (error)
                StartWriteAndFinish(&m_response, grpc::WriteOptions(), grpc::Status::OK); // just send the exception
            else
                StartWrite(&m_response);
        }

        Er::Log2::ILogger* m_log;
        Er::Ipc::IService::Ptr m_service;
        Er::Ipc::IService::StreamId m_streamId = {};
        erebus::ServiceReply m_response;
    };

    Er::Ipc::IService::Ptr findService(const std::string& id) const;
    static Er::PropertyBag unmarshalArgs(const erebus::ServiceRequest* request, Er::IPropertyMapping* mapping, std::string_view context);
    static void marshalReplyProps(const Er::PropertyBag& props, erebus::ServiceReply* reply);
    static void marshalException(erebus::ServiceReply* reply, const std::exception& e);
    static void marshalException(erebus::ServiceReply* reply, const Er::Exception& e);

    Er::Ipc::Grpc::ServerArgs m_params;
    std::unique_ptr<grpc::Server> m_server;
    mutable std::shared_mutex m_servicesLock;
    std::unordered_map<std::string, Er::Ipc::IService::Ptr> m_services;
};

} // namespace Erp::Ipc::Grpc {}