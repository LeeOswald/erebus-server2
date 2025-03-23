#pragma once

#include <erebus/system/exception.hxx>
#include <erebus/system/property_bag.hxx>
#include <erebus/system/result.hxx>

namespace Er::Ipc
{

struct IClient
{
    using CallId = uintptr_t;

    struct IReceiver
    {
        virtual void receive(CallId callId, Er::PropertyBag&& result) = 0;
        virtual void receive(CallId callId, Er::Exception&& exception) = 0;
        virtual void receive(CallId callId, Er::ResultCode result, std::string&& message) = 0;

    protected:
        virtual ~IReceiver() = default;
    };

    struct IStreamReceiver
    {
        enum class Result
        {
            Continue,
            Cancel
        };

        virtual Result receive(CallId callId, Er::PropertyBag&& result) = 0;
        virtual Result receive(CallId callId, Er::Exception&& exception) = 0;
        virtual void finish(CallId callId, Er::ResultCode result, std::string&& message) = 0;
        virtual void finish(CallId callId) = 0;

    protected:
        virtual ~IStreamReceiver() = default;
    };

    using Ptr = std::unique_ptr<IClient>;

    virtual void ping(std::optional<std::chrono::milliseconds> timeout = std::nullopt) = 0;
    virtual void request(CallId callId, std::string_view request, const Er::PropertyBag& args, IReceiver* receiver, std::optional<std::chrono::milliseconds> timeout = std::nullopt) = 0;
    virtual void requestStream(CallId callId, std::string_view request, const Er::PropertyBag& args, IStreamReceiver* receiver) = 0;

    virtual ~IClient() = default;
};


} // namespace Er::Ipc {}