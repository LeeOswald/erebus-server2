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
        virtual ~ICompletion() {}

        virtual void handlePropertyMappingExpired() = 0;
        virtual void handleTransportError(Er::ResultCode result, std::string&& message) = 0;
    };

    struct IPingCompletion
        : public ICompletion
    {
        using Ptr = std::shared_ptr<IPingCompletion>;

        virtual void handleReply(std::size_t payloadSize, std::chrono::milliseconds rtt) = 0;
    };

    struct ICallCompletion
        : public ICompletion
    {
        using Ptr = std::shared_ptr<ICallCompletion>;

        virtual void handleReply(Er::PropertyBag&& reply) = 0;
        virtual void handleException(Er::Exception&& exception) = 0;
    };

    struct IGetPropertyMappingCompletion
        : public ICompletion
    {
        using Ptr = std::shared_ptr<IGetPropertyMappingCompletion>;

        virtual CallbackResult handleProperty(std::uint32_t id, Er::PropertyType type, const std::string& name, const std::string& readableName) = 0;
    };

    struct IPutPropertyMappingCompletion
        : public ICompletion
    {
        using Ptr = std::shared_ptr<IPutPropertyMappingCompletion>;
    };

    struct IStreamCompletion
        : public ICompletion
    {
        using Ptr = std::shared_ptr<IStreamCompletion>;

        virtual CallbackResult handleFrame(Er::PropertyBag&& frame) = 0;
        virtual CallbackResult handleException(Er::Exception&& exception) = 0;
    };

    using Ptr = std::unique_ptr<IClient>;

    virtual void ping(std::size_t payloadSize, IPingCompletion::Ptr handler) = 0;
    virtual void getPropertyMapping(IGetPropertyMappingCompletion::Ptr handler) = 0;
    virtual void putPropertyMapping(IPutPropertyMappingCompletion::Ptr handler) = 0;
    virtual void adddPropertyMapping(const Er::PropertyInfo* pi) = 0;
    virtual void call(std::string_view request, const Er::PropertyBag& args, ICallCompletion::Ptr handler) = 0;
    virtual void stream(std::string_view request, const Er::PropertyBag& args, IStreamCompletion::Ptr handler) = 0;

    virtual ~IClient() {};
};


} // namespace Er::Ipc {}