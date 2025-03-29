#include "common.hpp"

#include <erebus/ipc/grpc/grpc_client.hxx>
#include <erebus/ipc/grpc/grpc_server.hxx>
#include <erebus/system/exception.hxx>

#include <thread>
#include <vector>

namespace
{


using SimpleCompletion = CompletionBase<Er::Ipc::IClient::ICompletion>;


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
    : public TestClientBase
{
public:
    TestCall() = default;

    void startServer()
    {
        TestClientBase::startServer();

        auto service = std::make_shared<TestService>();
        service->registerService(m_server.get());
    }

    bool putPropertyMapping(std::size_t client)
    {
        ErAssert(client < m_clients.size());

        auto completion = std::make_shared<SimpleCompletion>();

        m_clients[client]->putPropertyMapping(completion);
        return completion->wait() && completion->success();
    }

    bool getPropertyMapping(std::size_t client)
    {
        ErAssert(client < m_clients.size());

        auto completion = std::make_shared<SimpleCompletion>();

        m_clients[client]->getPropertyMapping(completion);
        return completion->wait() && completion->success();
    }
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
    startClient(1);

    {
        auto completion = std::make_shared<CallCompletion>();

        Er::PropertyBag args;
        args.push_back(Er::Property(uint64_t(12), Er::Unspecified::UInt64));
        args.push_back(Er::Property(std::string("Hello"), Er::Unspecified::String));

        m_clients.front()->call("echo", args, completion);

        ASSERT_TRUE(completion->wait());

        EXPECT_FALSE(completion->error());
        EXPECT_TRUE(completion->serverPropertyMappingExpired());
    }

    ASSERT_TRUE(putPropertyMapping(0));

    {
        auto completion = std::make_shared<CallCompletion>();

        Er::PropertyBag args;
        args.push_back(Er::Property(uint64_t(12), Er::Unspecified::UInt64));
        args.push_back(Er::Property(std::string("Hello"), Er::Unspecified::String));

        m_clients.front()->call("echo", args, completion);

        ASSERT_TRUE(completion->wait());

        EXPECT_FALSE(completion->error());
        EXPECT_FALSE(completion->serverPropertyMappingExpired());
        EXPECT_TRUE(completion->clientPropertyMappingExpired());
    }

    ASSERT_TRUE(getPropertyMapping(0));

    {
        auto completion = std::make_shared<CallCompletion>();

        Er::PropertyBag args;
        args.push_back(Er::Property(uint64_t(12), Er::Unspecified::UInt64));
        args.push_back(Er::Property(std::string("Hello"), Er::Unspecified::String));

        m_clients.front()->call("echo", args, completion);

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
    startClient(1);

    auto completion = std::make_shared<CallCompletion>();

    Er::PropertyBag args;
    args.push_back(Er::Property(uint64_t(12), Er::Unspecified::UInt64));
    args.push_back(Er::Property(std::string("Hello"), Er::Unspecified::String));

    m_clients.front()->call("bark", args, completion);

    ASSERT_TRUE(completion->wait());

    EXPECT_TRUE(completion->error());
    EXPECT_EQ(*completion->error(), Er::Result::Unimplemented);
    
}

TEST_F(TestCall, Exception)
{
    startServer();
    startClient(1);
    ASSERT_TRUE(putPropertyMapping(0));
    ASSERT_TRUE(getPropertyMapping(0));

    auto completion = std::make_shared<CallCompletion>();

    Er::PropertyBag args;
    args.push_back(Er::Property(uint64_t(12), Er::Unspecified::UInt64));
    args.push_back(Er::Property(std::string("Hello"), Er::Unspecified::String));

    m_clients.front()->call("throws", args, completion);

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

TEST_F(TestCall, ConcurrentCall)
{
    struct ClientWorker
    {
        Er::Ipc::IClient* client;
        long id;
        long callCount;
        std::jthread worker;
        std::vector<std::string> requests;
        std::vector<std::shared_ptr<CallCompletion>> completions;
        
        ~ClientWorker()
        {
            Er::Log2::info(Er::Log2::get(), "~ClientWorker({})", id);
        }

        ClientWorker(Er::Ipc::IClient* client, long id, long callCount)
            : client(client)
            , id(id)
            , callCount(callCount)
            , worker([this]() { run();  })
        {
            Er::Log2::info(Er::Log2::get(), "ClientWorker({})", id);
        }

        void run()
        {
            for (long i = 0; i < callCount; ++i)
            {
                auto c = std::make_shared<CallCompletion>();

                auto s = Er::format("Call[{}][{}]", id, i);
                Er::PropertyBag args;
                args.push_back(Er::Property(s, Er::Unspecified::String));

                client->call("echo", args, c);

                requests.push_back(std::move(s));
                completions.push_back(c);
            }
        }

        bool wait()
        {
            long succeeded = 0;
            for (long i = 0; i < callCount; ++i)
            {
                if (completions[i]->wait())
                {
                    ++succeeded;
                }
                else
                {
                    Er::Log2::error(Er::Log2::get(), "Call[{}][{}] did not complete", id, i);
                }
            }

            return succeeded == callCount;
        }

        bool check()
        {
            long succeeded = 0;
            for (long i = 0; i < callCount; ++i)
            {
                if (!completions[i]->error())
                {
                    if (completions[i]->reply)
                    {
                        auto& reply = *completions[i]->reply;

                        auto v = Er::get<std::string>(reply, Er::Unspecified::String);
                        if (v)
                        {
                            auto s = Er::format("Call[{}][{}]", id, i);
                            if (*v == s)
                            {
                                ++succeeded;
                            }
                            else
                            {
                                Er::Log2::error(Er::Log2::get(), "Call[{}][{}] reply was [{}] instead of [{}]", *v, s);
                            }
                        }
                        else
                        {
                            Er::Log2::error(Er::Log2::get(), "Call[{}][{}] did not yield a reply string");
                        }
                    }
                    else
                    {
                        Er::Log2::error(Er::Log2::get(), "Call[{}][{}] did not yield a reply");
                    }
                }
                else
                {
                    Er::Log2::error(Er::Log2::get(), "Call[{}][{}] failed: {} ({})", id, i, *completions[i]->error(), completions[i]->errorMessage());
                }
            }

            return succeeded == callCount;
        }
    };

    const long clientCount = 5;
    const long callCount = 10;

    startServer();
    startClient(clientCount);

    for (long i = 0; i < clientCount; ++i)
    {
        ASSERT_TRUE(putPropertyMapping(i));
        ASSERT_TRUE(getPropertyMapping(i));
    }

    std::vector<std::unique_ptr<ClientWorker>> clients;
    clients.reserve(clientCount);
    for (long i = 0; i < clientCount; ++i)
    {
        clients.push_back(std::make_unique<ClientWorker>(m_clients[i].get(), i, callCount));
    }

    for (long i = 0; i < clientCount; ++i)
    {
        ASSERT_TRUE(clients[i]->wait());
    }

    for (long i = 0; i < clientCount; ++i)
    {
        ASSERT_TRUE(clients[i]->check());
    }
}