#include "common.hpp"

#include <mutex>
#include <unordered_map>

namespace
{

const std::int32_t ThrowNever = -1;
const std::int32_t ThrowInBeginStream = -2;
const std::int32_t ThrowInEndStream = -3;

const Er::PropertyInfo ReplyFrameCount{ Er::PropertyType::Int32, "Er.Test.Grpc.reply_frame_count", "Reply frame count" };
const Er::PropertyInfo ThrowInFrame{ Er::PropertyType::Int32, "Er.Test.Grpc.throw_in_frame", "Throw in frame #" };

const Er::PropertyInfo ReplyFrameIndex{ Er::PropertyType::Int32, "Er.Test.Grpc.reply_frame_index", "Reply frame index" };


class TestService
    : public Er::Ipc::IService
    , public std::enable_shared_from_this<TestService>
{
public:
    void registerService(Er::Ipc::IServer* container) override
    {
        container->registerService("simple_stream", shared_from_this());
    }

    void unregisterService(Er::Ipc::IServer* container) override
    {
        container->unregisterService(this);
    }

    Er::PropertyBag request(std::string_view request, std::uint32_t clientId, const Er::PropertyBag& args) override
    {
        ErThrow(Er::format("Unsupported request {}", request));
    }

    StreamId beginStream(std::string_view request, std::uint32_t clientId, const Er::PropertyBag& args) override
    {
        if (request == "simple_stream")
            return simpleStream(clientId, args);

        ErThrow(Er::format("Unsupported request {}", request));
    }

    void endStream(StreamId id)
    {
        auto* s = findStream(id);
        ErAssert(s);

        if (s->type == SimpleStream::Type)
        {
            return endSimpleStream(static_cast<SimpleStream*>(s));
        }

        removeStream(id);
    }

    Er::PropertyBag next(StreamId id)
    {
        auto* s = findStream(id);
        ErAssert(s);

        if (s->type == SimpleStream::Type)
            return nextSimpleStream(static_cast<SimpleStream*>(s));
        
        return {};
    }

private:
    struct StreamBase
    {
        int type;
        StreamId id;

        StreamBase(int type, StreamId id)
            : type(type)
            , id(id)
        {
        }

        virtual ~StreamBase() {}
    };

    struct SimpleStream
        : public StreamBase
    {
        static constexpr int Type = 1;

        Er::PropertyBag args;
        std::int32_t throwInFrame;
        std::int32_t frameCount;
        std::int32_t nextFrame = 0;

        SimpleStream(StreamId id, const Er::PropertyBag& args, std::int32_t throwInFrame, std::int32_t frameCount)
            : StreamBase(Type, id)
            , throwInFrame(throwInFrame)
            , frameCount(frameCount)
            , args(args)
        {
        }
    };

    StreamBase* findStream(StreamId id)
    {
        StreamBase* s = nullptr;
        
        std::unique_lock l(m_mutex);

        auto it = m_streams.find(id);
        if (it == m_streams.end())
            ErThrow(Er::format("Unknown stream {}", id));

        s = it->second.get();
        
        return s;
    }

    void removeStream(StreamId id)
    {
        std::unique_lock l(m_mutex);

        auto it = m_streams.find(id);
        if (it != m_streams.end())
            m_streams.erase(it);
    }

    StreamId simpleStream(std::uint32_t clientId, const Er::PropertyBag& args)
    {
        auto fc = Er::get<std::int32_t>(args, ReplyFrameCount);
        if (!fc)
            throw Er::Exception(std::source_location::current(), "ReplyFrameCount property not found");

        auto tf = Er::get<std::int32_t>(args, ThrowInFrame);
        if (!tf)
            throw Er::Exception(std::source_location::current(), "ThrowInFrame property not found");

        if (*tf == ThrowInBeginStream)
            throw Er::Exception(std::source_location::current(), "No way you can start a stream");

        std::unique_lock l(m_mutex);

        auto id = m_nextStreamId++;
        auto stream = std::make_unique<SimpleStream>(id, args, *tf, *fc);
        m_streams.insert({ id, std::move(stream) });

        return id;
    }

    Er::PropertyBag nextSimpleStream(SimpleStream* s)
    {
        if (s->throwInFrame == s->frameCount)
        {
            throw Er::Exception(std::source_location::current(), "No way you can continue a stream");
        }

        Er::PropertyBag bag;

        if (s->nextFrame < s->frameCount)
        {
            for (auto& prop : s->args)
            {
                bag.push_back(prop);
            }

            bag.push_back(Er::Property(s->nextFrame, ReplyFrameIndex));

            ++s->nextFrame;
        }

        return bag;
    }

    void endSimpleStream(SimpleStream* s)
    {
        auto v = s->throwInFrame;
        removeStream(s->id);

        if (v == ThrowInEndStream)
        {
            throw Er::Exception(std::source_location::current(), "No way you can end a stream");
        }
    }

    std::mutex m_mutex;
    std::unordered_map<StreamId, std::unique_ptr<StreamBase>> m_streams;
    StreamId m_nextStreamId = 0;
};


class TestStream
    : public TestClientBase
{
public:
    TestStream() = default;

    void startServer()
    {
        TestClientBase::startServer();

        auto service = std::make_shared<TestService>();
        service->registerService(m_server.get());
    }
};


struct StreamCompletion
    : public CompletionBase<Er::Ipc::IClient::IStreamCompletion>
{
    StreamCompletion(std::uint32_t frameCount)
        : frameCount(frameCount)
    {
        frames.resize(frameCount);
        exceptions.resize(frameCount);
    }

    Er::CallbackResult handleFrame(Er::PropertyBag&& frame) override
    {
        ++currentFrame;
        frames[currentFrame] = std::move(frame);
        ++receivedFrames;
        
        if (currentFrame + 1 >= std::int32_t(frameCount))
            m_complete.setAndNotifyOne(true);

        return Er::CallbackResult::Continue;
    }

    Er::CallbackResult handleException(Er::Exception&& exception) override
    {
        ++currentFrame;
        exceptions[currentFrame] = std::move(exception);
        ++receivedExceptions;
        
        if (currentFrame + 1 >= std::int32_t(frameCount))
            m_complete.setAndNotifyOne(true);

        return Er::CallbackResult::Continue;
    }

    const std::uint32_t frameCount;
    std::int32_t currentFrame = -1;
    std::uint32_t receivedFrames = 0;
    std::uint32_t receivedExceptions = 0;
    std::vector<Er::PropertyBag> frames;
    std::vector<Er::Exception> exceptions;
};

} // namespace {}


TEST_F(TestStream, NormalStream)
{
    startServer();
    startClient(1);

    {
        auto completion = std::make_shared<StreamCompletion>(10);

        Er::PropertyBag args;
        args.push_back(Er::Property(int64_t(-12), Er::Unspecified::Int64));
        args.push_back(Er::Property(std::string("Bye"), Er::Unspecified::String));
        args.push_back(Er::Property(int32_t(10), ReplyFrameCount));
        args.push_back(Er::Property(int32_t(ThrowNever), ThrowInFrame));

        m_clients.front()->stream("simple_stream", args, completion);

        ASSERT_TRUE(completion->wait());

        EXPECT_FALSE(completion->error());
        EXPECT_TRUE(completion->serverPropertyMappingExpired());
    }

    ASSERT_TRUE(putPropertyMapping(0));

    {
        auto completion = std::make_shared<StreamCompletion>(10);

        Er::PropertyBag args;
        args.push_back(Er::Property(int64_t(-12), Er::Unspecified::Int64));
        args.push_back(Er::Property(std::string("Bye"), Er::Unspecified::String));
        args.push_back(Er::Property(int32_t(10), ReplyFrameCount));
        args.push_back(Er::Property(int32_t(ThrowNever), ThrowInFrame));

        m_clients.front()->stream("simple_stream", args, completion);

        ASSERT_TRUE(completion->wait());

        EXPECT_FALSE(completion->error());
        EXPECT_FALSE(completion->serverPropertyMappingExpired());
        EXPECT_TRUE(completion->clientPropertyMappingExpired());
    }

    ASSERT_TRUE(getPropertyMapping(0));

    {
        const std::uint32_t frameCount = 10;

        auto completion = std::make_shared<StreamCompletion>(frameCount);

        Er::PropertyBag args;
        args.push_back(Er::Property(int64_t(-12), Er::Unspecified::Int64));
        args.push_back(Er::Property(std::string("Bye"), Er::Unspecified::String));
        args.push_back(Er::Property(int32_t(frameCount), ReplyFrameCount));
        args.push_back(Er::Property(int32_t(ThrowNever), ThrowInFrame));

        m_clients.front()->stream("simple_stream", args, completion);

        ASSERT_TRUE(completion->wait());

        EXPECT_FALSE(completion->error());
        EXPECT_FALSE(completion->serverPropertyMappingExpired());
        EXPECT_FALSE(completion->clientPropertyMappingExpired());

        EXPECT_EQ(completion->receivedFrames, frameCount);
        EXPECT_EQ(completion->receivedExceptions, 0);

        for (std::uint32_t i = 0; i < frameCount; ++i)
        {
            ASSERT_FALSE(completion->frames[i].empty());
            EXPECT_FALSE(completion->exceptions[i]);

            auto& props = completion->frames[i];
            EXPECT_EQ(props.size(), 5);

            auto rfi = Er::get<std::int32_t>(props, ReplyFrameIndex);
            ASSERT_TRUE(!!rfi);
            EXPECT_EQ(*rfi, i);

            auto i64 = Er::get<std::int64_t>(props, Er::Unspecified::Int64);
            ASSERT_TRUE(!!i64);
            EXPECT_EQ(*i64, -12);

            auto s = Er::get<std::string>(props, Er::Unspecified::String);
            ASSERT_TRUE(!!s);
            EXPECT_STREQ(s->c_str(), "Bye");

            auto fc = Er::get<std::int32_t>(props, ReplyFrameCount);
            ASSERT_TRUE(!!fc);
            EXPECT_EQ(*fc, frameCount);

            auto tif = Er::get<std::int32_t>(props, ThrowInFrame);
            ASSERT_TRUE(!!tif);
            EXPECT_EQ(*tif, ThrowNever);
        }
    }
}

TEST_F(TestStream, ConcurrentStreams)
{
    struct ClientWorker
    {
        Er::Ipc::IClient* client;
        std::int64_t id;
        std::uint32_t frameCount;
        std::shared_ptr<StreamCompletion> completion;
        std::jthread worker;
        
        ~ClientWorker()
        {
            ErLogDebug("~ClientWorker({})", id);
        }

        ClientWorker(Er::Ipc::IClient* client, std::int64_t id, std::uint32_t frameCount)
            : client(client)
            , id(id)
            , frameCount(frameCount)
            , completion(std::make_shared<StreamCompletion>(frameCount))
            , worker([this]() { run();  })
        {
            ErLogDebug("ClientWorker({})", id);
        }

        void run()
        {
            Er::PropertyBag args;
            args.push_back(Er::Property(id, Er::Unspecified::Int64));
            args.push_back(Er::Property(int32_t(frameCount), ReplyFrameCount));
            args.push_back(Er::Property(int32_t(ThrowNever), ThrowInFrame));

            client->stream("simple_stream", args, completion);
        }

        bool wait()
        {
            if (!completion->wait())
            {
                ErLogError("Stream {} did not complete", id);
                return false;
            }

            return true;
        }

        bool check()
        {
            if (completion->error())
            {
                ErLogError("Stream {} completed with an error {} ({})", id, *completion->error(), completion->errorMessage());
                return false;
            }

            if (completion->serverPropertyMappingExpired())
            {
                ErLogError("Stream {} has server mapping expired", id);
                return false;
            }

            if (completion->clientPropertyMappingExpired())
            {
                ErLogError("Stream {} has client mapping expired", id);
                return false;
            }

            if (completion->receivedFrames != frameCount)
            {
                ErLogError("Stream {} received {} frames out of {}", id, completion->receivedFrames, frameCount);
                return false;
            }

            if (completion->receivedExceptions != 0)
            {
                ErLogError("Stream {} has {} exceptions", id, completion->receivedExceptions);
                return false;
            }

            for (std::uint32_t i = 0; i < frameCount; ++i)
            {
                if (completion->frames[i].empty())
                {
                    ErLogError("Stream {} frame #{} empty", id, i);
                    return false;
                }

                if (completion->exceptions[i])
                {
                    ErLogError("Stream {} frame #{} has an exception", id, i);
                    return false;
                }

                auto& props = completion->frames[i];
                if (props.size() != 4)
                {
                    ErLogError("Stream {} frame #{} has {} props instead of {}", id, i, props.size(), 4);
                    return false;
                }

                auto rfi = Er::get<std::int32_t>(props, ReplyFrameIndex);
                if (!rfi)
                {
                    ErLogError("Stream {} frame #{} has no ReplyFrameIndex", id, i);
                    return false;
                }

                if (*rfi != i)
                {
                    ErLogError("Stream {} frame #{} has ReplyFrameIndex {}", id, i, *rfi);
                    return false;
                }

                auto i64 = Er::get<std::int64_t>(props, Er::Unspecified::Int64);
                if (!i64)
                {
                    ErLogError("Stream {} frame #{} has no Er::Unspecified::Int64", id, i);
                    return false;
                }

                if (*i64 != id)
                {
                    ErLogError("Stream {} frame #{} has Er::Unspecified::Int64 {}", id, i, *i64);
                    return false;
                }

                auto fc = Er::get<std::int32_t>(props, ReplyFrameCount);
                if (!fc)
                {
                    ErLogError("Stream {} frame #{} has no ReplyFrameCount", id, i);
                    return false;
                }

                if (*fc != frameCount)
                {
                    ErLogError("Stream {} frame #{} has ReplyFrameCount {}", id, i, *fc);
                    return false;
                }

                auto tif = Er::get<std::int32_t>(props, ThrowInFrame);
                if (!tif)
                {
                    ErLogError("Stream {} frame #{} has no ThrowInFrame", id, i);
                    return false;
                }

                if (*tif != ThrowNever)
                {
                    ErLogError("Stream {} frame #{} has ThrowInFrame {}", id, i, *tif);
                    return false;
                }
            }

            return true;
        }
    };

    const long clientCount = 5;
    const std::uint32_t frameCount = 1000;

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
        clients.push_back(std::make_unique<ClientWorker>(m_clients[i].get(), i, frameCount));
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