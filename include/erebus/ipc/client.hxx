#pragma once

#include <erebus/ipc/ipc.hxx>

#include <erebus/system/exception.hxx>
#include <erebus/system/property_bag.hxx>
#include <erebus/system/result.hxx>

#include <chrono>

namespace Er::Ipc
{

struct IClient
{
    struct Timings
    {
        std::optional<std::chrono::microseconds> roundTripTime;
        std::optional<std::chrono::microseconds> processingTime;
    };

    struct ICompletion
    {
        using Ptr = std::shared_ptr<ICompletion>;

        virtual ~ICompletion() {}

        virtual void done() noexcept = 0;
        virtual void onTransportError(Er::ResultCode result, std::string&& message) noexcept = 0;
        virtual void onException(Er::Exception&& exception) noexcept = 0;
    };

    struct ICallCompletion
        : public ICompletion
    {
        using Ptr = std::shared_ptr<ICallCompletion>;

        virtual void onReply(Er::PropertyBag&& reply, const Timings& timings) = 0;
    };

    struct IStreamCompletion
        : public ICompletion
    {
        using Ptr = std::shared_ptr<IStreamCompletion>;

        virtual CallbackResult onFrame(Er::PropertyBag&& frame, const Timings& timings) = 0;
    };

    using Ptr = std::unique_ptr<IClient>;

    virtual void call(std::string_view request, const Er::PropertyBag& args, ICallCompletion::Ptr handler, std::chrono::milliseconds timeout) = 0;
    virtual void stream(std::string_view request, const Er::PropertyBag& args, IStreamCompletion::Ptr handler) = 0;

    virtual ~IClient() {};
};


} // namespace Er::Ipc {}