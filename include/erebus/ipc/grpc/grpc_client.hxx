#pragma once

#include <erebus/system/logger2.hxx>
#include <erebus/ipc/client.hxx>

#include <chrono>


#if ER_WINDOWS
#ifdef ER_GRPC_CLIENT_EXPORTS
#define ER_GRPC_CLIENT_EXPORT __declspec(dllexport)
#else
#define ER_GRPC_CLIENT_EXPORT __declspec(dllimport)
#endif
#else
#define ER_GRPC_CLIENT_EXPORT __attribute__((visibility("default")))
#endif


namespace Er::Ipc::Grpc
{


struct ChannelSettings
{
    std::string endpoint;
    bool useTls;
    std::string rootCertificates;
    std::string certificate;
    std::string privateKey;
    bool keepAlive = true;

    explicit ChannelSettings(std::string_view endpoint)
        : endpoint(endpoint)
        , useTls(false)
    {
    }

    explicit ChannelSettings(
        std::string_view endpoint,
        bool useTls,
        std::string_view rootCertificate,
        std::string_view certificate,
        std::string_view key
    )
        : endpoint(endpoint)
        , useTls(useTls)
        , rootCertificates(rootCertificates)
        , certificate(certificate)
        , privateKey(privateKey)
    {
    }
};

using ChannelPtr = std::shared_ptr<void>;


ER_GRPC_CLIENT_EXPORT ChannelPtr createChannel(const ChannelSettings& params);

ER_GRPC_CLIENT_EXPORT IClient::Ptr createClient(ChannelPtr channel, Er::Log2::ILogger::Ptr log);

} // namespace Er::Ipc::Grpc {}