#include "common.hpp"

#include <erebus/ipc/grpc/grpc_client.hxx>
#include <erebus/ipc/grpc/grpc_server.hxx>
#include <erebus/system/exception.hxx>


namespace
{

class TestService
    : public Er::Ipc::IService
    , public std::enable_shared_from_this<TestService>
{
public:
    void registerService(Er::Ipc::IServer* container) override
    {
        container->registerService("echo", shared_from_this());
        container->registerService("throws", shared_from_this());
    }

    void unregisterService(Er::Ipc::IServer* container) override
    {
        container->unregisterService(this);
    }

    Er::PropertyBag request(std::string_view request, std::string_view cookie, const Er::PropertyBag& args) override
    {
        if (request == "echo")
            return echo(cookie, args);
        else if (request == "throws")
            return throws(cookie, args);

        ErThrow(Er::format("Unsupported request {}", request));
    }

    StreamId beginStream(std::string_view request, std::string_view cookie, const Er::PropertyBag& args) override
    {
        ErThrow(Er::format("Unsupported request {}", request));
    }

    void endStream(StreamId id)
    {
    }

    Er::PropertyBag next(StreamId id)
    {
        return {};
    }

private:
    Er::PropertyBag echo(std::string_view cookie, const Er::PropertyBag& args)
    {
        return args;
    }

    Er::PropertyBag throws(std::string_view cookie, const Er::PropertyBag& args)
    {
        Er::Exception e(std::source_location::current(), "This is my exception");
        for (auto& prop : args)
        {
            e.add(prop);
        }

        throw e;
    }
};


class TestCall
    : public testing::Test
{
public:
    ~TestCall()
    {
        stopClient();
        stopServer();
    }

    TestCall()
        : m_port(g_serverPort)
    {
    }

    void startServer()
    {
        Er::Ipc::Grpc::ServerArgs args(Er::Log2::get());
        args.endpoints.push_back(Er::Ipc::Grpc::ServerArgs::Endpoint(Er::format("127.0.0.1:{}", m_port)));

        m_server = Er::Ipc::Grpc::create(args);

        auto service = std::make_shared<TestService>();
        service->registerService(m_server.get());
    }

    void stopServer()
    {
        m_server.reset();
    }

    void startClient()
    {
        Er::Ipc::Grpc::ChannelSettings args(Er::format("127.0.0.1:{}", m_port));

        auto channel = Er::Ipc::Grpc::createChannel(args);

        m_client = Er::Ipc::Grpc::createClient(args, channel, Er::Log2::get());
    }

    void stopClient()
    {
        m_client.reset();
    }

    bool putPropertyMapping()
    {
        auto completion = std::make_shared<CompletionBase<Er::Ipc::IClient::ICompletion>>();

        m_client->putPropertyMapping(completion);
        return completion->wait() && completion->success();
    }

    bool getPropertyMapping()
    {
        auto completion = std::make_shared<CompletionBase<Er::Ipc::IClient::ICompletion>>();

        m_client->getPropertyMapping(completion);
        return completion->wait() && completion->success();
    }

protected:
    std::uint16_t m_port;
    Er::Ipc::IServer::Ptr m_server;
    Er::Ipc::IClient::Ptr m_client;
};


struct CallCompletion
    : public CompletionBase<Er::Ipc::IClient::ICallCompletion>
{
    CallCompletion()
    {
    }

    void handleReply(Er::PropertyBag&& reply) override
    {
        this->reply = std::move(reply);
        m_complete.setAndNotifyOne(true);
    }

    void handleException(Er::Exception&& exception) override
    {
        this->exception = std::move(exception);
        m_complete.setAndNotifyOne(true);
    }

    std::optional<Er::PropertyBag> reply;
    std::optional<Er::Exception> exception;
};

} // namespace {}


TEST_F(TestCall, NormalCall)
{
    startServer();
    startClient();

    {
        auto completion = std::make_shared<CallCompletion>();

        Er::PropertyBag args;
        args.push_back(Er::Property(uint64_t(12), Er::Unspecified::UInt64));
        args.push_back(Er::Property(std::string("Hello"), Er::Unspecified::String));

        m_client->call("echo", args, completion);

        ASSERT_TRUE(completion->wait());

        EXPECT_FALSE(completion->error());
        EXPECT_TRUE(completion->serverPropertyMappingExpired());
    }

    ASSERT_TRUE(putPropertyMapping());

    {
        auto completion = std::make_shared<CallCompletion>();

        Er::PropertyBag args;
        args.push_back(Er::Property(uint64_t(12), Er::Unspecified::UInt64));
        args.push_back(Er::Property(std::string("Hello"), Er::Unspecified::String));

        m_client->call("echo", args, completion);

        ASSERT_TRUE(completion->wait());

        EXPECT_FALSE(completion->error());
        EXPECT_FALSE(completion->serverPropertyMappingExpired());
        EXPECT_TRUE(completion->clientPropertyMappingExpired());
    }

    ASSERT_TRUE(getPropertyMapping());

    {
        auto completion = std::make_shared<CallCompletion>();

        Er::PropertyBag args;
        args.push_back(Er::Property(uint64_t(12), Er::Unspecified::UInt64));
        args.push_back(Er::Property(std::string("Hello"), Er::Unspecified::String));

        m_client->call("echo", args, completion);

        ASSERT_TRUE(completion->wait());

        EXPECT_FALSE(completion->error());
        EXPECT_FALSE(completion->serverPropertyMappingExpired());
        EXPECT_FALSE(completion->clientPropertyMappingExpired());

        ASSERT_TRUE(completion->reply);

        auto& reply = *completion->reply;
        ASSERT_EQ(reply.size(), 2);

        {
            auto v = Er::get<uint64_t>(reply, Er::Unspecified::UInt64);
            ASSERT_TRUE(v);

            EXPECT_EQ(*v, 12);
        }

        {
            auto v = Er::get<std::string>(reply, Er::Unspecified::String);
            ASSERT_TRUE(v);

            EXPECT_STREQ(v->c_str(), "Hello");
        }
    }
}

TEST_F(TestCall, NotImplemented)
{
    startServer();
    startClient();

    auto completion = std::make_shared<CallCompletion>();

    Er::PropertyBag args;
    args.push_back(Er::Property(uint64_t(12), Er::Unspecified::UInt64));
    args.push_back(Er::Property(std::string("Hello"), Er::Unspecified::String));

    m_client->call("bark", args, completion);

    ASSERT_TRUE(completion->wait());

    EXPECT_TRUE(completion->error());
    EXPECT_EQ(*completion->error(), Er::Result::Unimplemented);
    
}

TEST_F(TestCall, Exception)
{
    startServer();
    startClient();
    ASSERT_TRUE(putPropertyMapping());
    ASSERT_TRUE(getPropertyMapping());

    auto completion = std::make_shared<CallCompletion>();

    Er::PropertyBag args;
    args.push_back(Er::Property(uint64_t(12), Er::Unspecified::UInt64));
    args.push_back(Er::Property(std::string("Hello"), Er::Unspecified::String));

    m_client->call("throws", args, completion);

    ASSERT_TRUE(completion->wait());

    EXPECT_FALSE(completion->error());
    EXPECT_TRUE(completion->exception);

    auto& e = *completion->exception;
    auto& props = e.properties();

    ASSERT_EQ(props.size(), 2);

    {
        auto v = Er::get<uint64_t>(props, Er::Unspecified::UInt64);
        ASSERT_TRUE(v);

        EXPECT_EQ(*v, 12);
    }

    {
        auto v = Er::get<std::string>(props, Er::Unspecified::String);
        ASSERT_TRUE(v);

        EXPECT_STREQ(v->c_str(), "Hello");
    }
}
