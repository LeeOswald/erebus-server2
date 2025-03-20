#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <erebus/ipc/grpc/grpc_server.hxx>

#include <atomic>


namespace Er::Ipc::Grpc
{

namespace
{

Er::Log2::ILogger* g_log = nullptr;
std::atomic<long> g_initialized = 0;


} // namespace {}

ER_GRPC_EXPORT void initialize(Er::Log2::ILogger* log)
{
    if (g_initialized.fetch_add(1, std::memory_order_acq_rel) == 0)
    {
        g_log = log;

        ::grpc_init();

        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    }
}
    
ER_GRPC_EXPORT void finalize()
{
    if (g_initialized.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        ::grpc_shutdown();

        g_log = nullptr;
    }
}
    

} // namespace Er::Ipc::Grpc {}