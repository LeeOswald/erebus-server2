#pragma once

#include <erebus/ipc/server.hxx>
#include <erebus/system/logger2.hxx>

#include <vector>


#if ER_WINDOWS
    #ifdef ER_GRPC_EXPORTS
        #define ER_GRPC_EXPORT __declspec(dllexport)
    #else
        #define ER_GRPC_EXPORT __declspec(dllimport)
    #endif
#else
    #define ER_GRPC_EXPORT __attribute__((visibility("default")))
#endif

namespace Er::Ipc::Grpc
{


struct ServerArgs
{
    struct Endpoint
    {
        std::string endpoint;
        bool useTls = false;
        std::string rootCertificates;
        std::string certificate;
        std::string privateKey;
        
        Endpoint(const std::string& endpoint)
            : endpoint(endpoint)
        {
        }

        Endpoint(const std::string& endpoint, const std::string& rootCertificates, const std::string& certificate, const std::string& privateKey)
            : endpoint(endpoint)
            , useTls(true)
            , rootCertificates(rootCertificates)
            , certificate(certificate)
            , privateKey(privateKey)
        {
        }
    };

    Er::Log2::ILogger* log = nullptr;
    std::vector<Endpoint> endpoints;
    bool keepAlive = true;

    explicit ServerArgs(Er::Log2::ILogger* log) noexcept
        : log(log)
    {
    }
};    


ER_GRPC_EXPORT void initialize(Er::Log2::ILogger* log);
ER_GRPC_EXPORT void finalize();

[[nodiscard]] IServer::Ptr ER_GRPC_EXPORT create(const ServerArgs& params);
    

} // namespace Er::Ipc::Grpc {}