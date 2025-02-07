#include "common.hpp"

#include <erebus/system/program.hxx>

#if ER_WINDOWS
#include <erebus/system/logger/win32_debugger_sink2.hxx>
#endif
#include <erebus/system/logger/simple_formatter2.hxx>
#include <erebus/system/logger/ostream_sink2.hxx>

int InstanceCounter::instances = 0;

namespace
{

class TestEventListener 
    : public testing::EmptyTestEventListener 
{
    void OnTestSuiteStart(const testing::TestSuite& suite) override 
    {
        Er::Log2::info(Er::Log2::get(), "[{}] ------------------", suite.name());
    }

    void OnTestSuiteEnd(const testing::TestSuite& suite) override
    {
    }

    void OnTestStart(const testing::TestInfo& test) override 
    {
        m_failed = 0;
        Er::Log2::info(Er::Log2::get(), "[RUN {}]", test.name());
    }

    void OnTestPartResult(const testing::TestPartResult& result) override 
    {
        if (result.failed())
        {
            ++m_failed;
            Er::Log2::info(Er::Log2::get(), "[    FAILED] in {}:{}\n{}", result.file_name(), result.line_number(), result.summary());
        }
    }
    
    void OnTestEnd(const testing::TestInfo& test) override 
    {
        if (!m_failed)
        {
            Er::Log2::info(Er::Log2::get(), "[    OK]");
        }
    }

private:
    unsigned m_failed = 0;
};

class TestApplication final
    : public Er::Program
{
public:
    TestApplication() noexcept = default;

private:
    void addCmdLineOptions(boost::program_options::options_description& options) override
    {
        options.add_options()
            ("gtest_list_tests", "list all tests")
            ("gtest_filter", "run specific tests")
            ;
    }

    void addLoggers(Er::Log2::ITee* main) override
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

    int run(int argc, char** argv) override
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
};

} // namespace {}


int main(int argc, char** argv)
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

    try
    {

        TestApplication app;

        auto resut = app.exec(argc, argv);

        return resut;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unexpected exception" << std::endl;
    }

    return -1;
}

