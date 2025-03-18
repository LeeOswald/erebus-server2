#include <absl/log/log_sink_registry.h>

#include <google/protobuf/message_lite.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <erebus/ipc/grpc/grpc_server.hxx>

#include <atomic>

#include <boost/noncopyable.hpp>


namespace Er::Ipc::Grpc
{

namespace
{

Er::Log2::ILogger* g_log = nullptr;
std::atomic<long> g_initialized = 0;


class GrpcLogSink
    : public absl::LogSink
    , public boost::noncopyable
{
public:
    void Send(const absl::LogEntry& entry) override
    {
        if (g_log)
        {
            Er::Log2::Level level = Er::Log2::Level::Debug;
            switch (entry.log_severity())
            {
            case absl::LogSeverity::kInfo: level = Er::Log2::Level::Info; break;
            case absl::LogSeverity::kWarning: level = Er::Log2::Level::Warning; break;
            case absl::LogSeverity::kError: level = Er::Log2::Level::Error; break;
            case absl::LogSeverity::kFatal: level = Er::Log2::Level::Fatal; break;
            }

            Er::Log2::write("gRPC", g_log, level, "{}", entry.text_message());
        }
    }

    void Flush() override
    {
        if (g_log)
            g_log->flush();
    }
};


GrpcLogSink g_abslLogSink;


} // namespace {}

ER_GRPC_EXPORT void initialize(Er::Log2::ILogger* log)
{
    if (g_initialized.fetch_add(1, std::memory_order_acq_rel) == 0)
    {
        g_log = log;

        ::grpc_init();

        absl::AddLogSink(&g_abslLogSink);

        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    }
}
    
ER_GRPC_EXPORT void finalize()
{
    if (g_initialized.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        ::grpc_shutdown();
        google::protobuf::ShutdownProtobufLibrary();

        g_log = nullptr;
    }
}
    

} // namespace Er::Ipc::Grpc {}