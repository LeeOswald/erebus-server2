#pragma once

#include <erebus/system/exception.hxx>
#include <erebus/system/property_bag.hxx>
#include <erebus/system/result.hxx>

namespace Er::Ipc
{

struct IClient
{
    struct ICompletion
    {
        virtual void handleTransportError(void* context, Er::ResultCode result, std::string&& message) = 0;

    protected:
        virtual ~ICompletion() {}
    };

    struct IPingCompletion
        : public ICompletion
    {
        virtual void handleReply(void* context, std::chrono::milliseconds rtt) = 0;
    };

    struct ICallCompletion
        : public ICompletion
    {
        virtual void handleReply(const std::string& uri, void* context, Er::PropertyBag&& reply) = 0;
        virtual void handleException(const std::string& uri, void* context, Er::Exception&& exception) = 0;
    };

    struct IStreamCompletion
        : public ICompletion
    {
        enum class Should
        {
            Continue,
            Cancel
        };

        virtual Should handleFrame(const std::string& uri, void* context, Er::PropertyBag&& frame) = 0;
        virtual Should handleException(const std::string& uri, void* context, Er::Exception&& exception) = 0;
        virtual void handleEndOfStream(const std::string& uri, void* context) = 0;
    };

    using Ptr = std::unique_ptr<IClient>;

    virtual void ping(std::size_t payloadSize, IPingCompletion* handler, void* context) = 0;
    virtual void call(std::string_view request, const Er::PropertyBag& args, ICallCompletion* handler, void* context) = 0;
    virtual void stream(std::string_view request, const Er::PropertyBag& args, IStreamCompletion* handler, void* context) = 0;

    virtual ~IClient() {};
};


} // namespace Er::Ipc {}