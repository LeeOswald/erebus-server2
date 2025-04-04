#pragma once

#include <erebus/ipc/service.hxx>


namespace Er::Ipc
{

struct IServer
{
    using Ptr = std::unique_ptr<IServer>;

    virtual ~IServer() = default;
    virtual void registerService(std::string_view request, IService::Ptr service) = 0;
    virtual void unregisterService(IService* service) noexcept = 0;
};

} // namespace Er::Ipc {}