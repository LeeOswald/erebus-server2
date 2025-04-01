#pragma once

#include <erebus/system/property_bag.hxx>


namespace Er::Ipc
{

struct IServer;

struct IService
{
    using StreamId = uintptr_t;
    using Ptr = std::shared_ptr<IService>;

    virtual ~IService() = default;

    virtual void registerService(IServer* container) = 0;
    virtual void unregisterService(IServer* container) = 0;

    virtual Er::PropertyBag request(std::string_view request, std::uint32_t clientId, const Er::PropertyBag& args) = 0; 
    [[nodiscard]] virtual StreamId beginStream(std::string_view request, std::uint32_t clientId, const Er::PropertyBag& args) = 0;
    virtual void endStream(StreamId id) = 0;
    virtual Er::PropertyBag next(StreamId id) = 0;
};


} // namespace Er::Ipc {}