#pragma once

#include <erebus/ipc/ipc.hxx>
#include <erebus/system/property_bag.hxx>


namespace Er::Ipc
{

struct IServer;

struct IService
{
    using StreamId = uintptr_t;
    static constexpr StreamId InvalidStreamId = StreamId(-1);

    using Ptr = std::shared_ptr<IService>;

    virtual ~IService() = default;

    virtual void registerService(IServer* container) = 0;
    virtual void unregisterService(IServer* container) noexcept = 0;

    [[nodiscard]] virtual Er::PropertyBag request(std::string_view request, ClientId clientId, Er::PropertyBag&& args) = 0;
    [[nodiscard]] virtual StreamId beginStream(std::string_view request, ClientId clientId, Er::PropertyBag&& args) = 0;
    [[nodiscard]] virtual Er::PropertyBag next(StreamId id) = 0;
    virtual void endStream(StreamId id) noexcept = 0;
};


} // namespace Er::Ipc {}