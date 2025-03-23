#include <erebus/testing/test_application.hxx>
#include <erebus/testing/test_event_listener.hxx>

#if ER_WINDOWS
    #include <erebus/system/logger/win32_debugger_sink2.hxx>
#endif
#include <erebus/system/logger/simple_formatter2.hxx>
#include <erebus/system/logger/ostream_sink2.hxx>

#include <gtest/gtest.h>

namespace Erp::Testing
{


TestApplication::TestApplication(int options) noexcept
    : Er::Program(options)
{
#if ER_POSIX
    {
        // globally block signals so that child threads 
        // inherit signal mask with signals blocked 
        sigset_t mask;
        ::sigemptyset(&mask);
        ::sigaddset(&mask, SIGTERM);
        ::sigaddset(&mask, SIGHUP);
        ::sigaddset(&mask, SIGINT);
        ::sigaddset(&mask, SIGUSR1);
        ::sigaddset(&mask, SIGUSR2);
        ::sigprocmask(SIG_BLOCK, &mask, nullptr);
    }
#endif
}

void TestApplication::addCmdLineOptions(boost::program_options::options_description& options)
{
    options.add_options()
        ("gtest_list_tests", "list all tests")
        ("gtest_filter", "run specific tests")
        ;
}

void TestApplication::addLoggers(Er::Log2::ITee* main)
{
#if ER_WINDOWS
    if (Er::isDebuggerPresent())
    {
        auto sink = Er::Log2::makeDebuggerSink(
            Er::Log2::SimpleFormatter::make(Er::Log2::SimpleFormatter::Options{ Er::Log2::SimpleFormatter::Option::Lf }),
            Er::Log2::Filter{}
        );

        main->addSink("debugger", sink);
    }
#endif

    {
        auto sink = Er::Log2::makeOStreamSink(
            std::cout,
            Er::Log2::SimpleFormatter::make(Er::Log2::SimpleFormatter::Options{ Er::Log2::SimpleFormatter::Option::Lf }),
            [](const Er::Log2::Record* r)
        {
            return r->level() < Er::Log2::Level::Error;
        }
        );

        main->addSink("std::cout", sink);
    }

    {
        auto sink = Er::Log2::makeOStreamSink(
            std::cerr,
            Er::Log2::SimpleFormatter::make(Er::Log2::SimpleFormatter::Options{ Er::Log2::SimpleFormatter::Option::Lf }),
            [](const Er::Log2::Record* r)
        {
            return r->level() >= Er::Log2::Level::Error;
        }
        );

        main->addSink("std::cerr", sink);
    }
}

int TestApplication::run(int argc, char** argv)
{
#if ER_POSIX
    // unblock signals in the test thread
    {
        sigset_t mask;
        ::sigemptyset(&mask);
        ::sigaddset(&mask, SIGTERM);
        ::sigaddset(&mask, SIGINT);
        ::sigaddset(&mask, SIGUSR1);
        ::sigaddset(&mask, SIGUSR2);
        ::sigprocmask(SIG_UNBLOCK, &mask, nullptr);
    }
#endif
    ::testing::InitGoogleTest(&argc, argv);

    auto& listeners = testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());
    listeners.Append(new TestEventListener);

    return RUN_ALL_TESTS();
}


} // Erp::Testing{}