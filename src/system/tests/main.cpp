#include <erebus/system/program.hxx>

#include "common.hpp"

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
    void OnTestIterationStart(const testing::UnitTest& test, int iteration) override
    {
        Er::Log2::info(Er::Log2::get(), "[==========] Running {} from {}.", test.test_to_run_count(), test.test_suite_to_run_count());
    }

    void OnTestIterationEnd(const testing::UnitTest& test, int iteration) override
    {
        Er::Log2::info(Er::Log2::get(), "[==========] {} from {} ran ({} ms total).", test.test_to_run_count(), test.test_suite_to_run_count(), test.elapsed_time());

        Er::Log2::info(Er::Log2::get(), "[  PASSED  ] {}", test.successful_test_count());

        auto skipped_test_count = test.skipped_test_count();
        if (skipped_test_count > 0) 
        {
            Er::Log2::info(Er::Log2::get(), "[  SKIPPED ] {}", skipped_test_count);
            for (int i = 0; i < test.total_test_suite_count(); ++i) 
            {
                auto& test_suite = *test.GetTestSuite(i);
                if (!test_suite.should_run() || (test_suite.skipped_test_count() == 0)) 
                {
                    continue;
                }

                for (int j = 0; j < test_suite.total_test_count(); ++j) 
                {
                    auto& test_info = *test_suite.GetTestInfo(j);
                    if (!test_info.should_run() || !test_info.result()->Skipped()) 
                    {
                        continue;
                    }
                    Er::Log2::info(Er::Log2::get(), "[  SKIPPED ] {}.{}", test_suite.name(), test_info.name());
                }
            }
        }

        if (!test.Passed()) 
        {
            auto failed_test_count = test.failed_test_count();
            Er::Log2::info(Er::Log2::get(), "[  FAILED  ] {}", failed_test_count);

            for (int i = 0; i < test.total_test_suite_count(); ++i) 
            {
                auto& test_suite = *test.GetTestSuite(i);
                if (!test_suite.should_run() || (test_suite.failed_test_count() == 0)) 
                {
                    continue;
                }

                for (int j = 0; j < test_suite.total_test_count(); ++j) 
                {
                    auto& test_info = *test_suite.GetTestInfo(j);
                    if (!test_info.should_run() || !test_info.result()->Failed()) 
                    {
                        continue;
                    }
                    Er::Log2::info(Er::Log2::get(), "[  FAILED  ] {}.{}", test_suite.name(), test_info.name());

                    auto type_param = test_info.type_param();
                    auto value_param = test_info.value_param();

                    if (type_param)
                    {
                        Er::Log2::info(Er::Log2::get(), "TypeParam = {}", type_param);
                    }

                    if (value_param)
                    {
                        Er::Log2::info(Er::Log2::get(), "GetParam() = {}", value_param);
                    }
                }
            }
            Er::Log2::info(Er::Log2::get(), "{} FAILED TESTS", failed_test_count);
        }

        auto num_disabled = test.reportable_disabled_test_count();
        if (num_disabled) 
        {
            if (test.Passed()) 
            {
                Er::Log2::info(Er::Log2::get(), "");  // Add a spacer if no FAILURE banner is displayed.
            }

            Er::Log2::info(Er::Log2::get(), "  YOU HAVE {} DISABLED TESTS\n", num_disabled);
        }
    }

    void OnEnvironmentsSetUpStart(const testing::UnitTest&) override
    {
        Er::Log2::info(Er::Log2::get(), "[----------] Global test environment set-up.");
    }

    void OnEnvironmentsTearDownStart(const testing::UnitTest&) override
    {
        Er::Log2::info(Er::Log2::get(), "[----------] Global test environment tear-down");
    }

    void OnTestSuiteStart(const testing::TestSuite& suite) override 
    {
        Er::Log2::info(Er::Log2::get(), "[----------] {} from {}", suite.test_to_run_count(), suite.name());
    }

    void OnTestSuiteEnd(const testing::TestSuite& suite) override
    {
        Er::Log2::info(Er::Log2::get(), "[----------] {} from {} ({} ms total)\n", suite.test_to_run_count(), suite.name(), suite.elapsed_time());
    }

    void OnTestStart(const testing::TestInfo& test) override 
    {
        Er::Log2::info(Er::Log2::get(), "[ RUN      ] {}.{}", test.test_suite_name(), test.name());
    }

    void OnTestDisabled(const testing::TestInfo& test) override
    {
        Er::Log2::info(Er::Log2::get(), "[ DISABLED ] {}.{}", test.test_suite_name(), test.name());
    }

    void OnTestPartResult(const testing::TestPartResult& result) override 
    {
        switch (result.type()) 
        {
            // If the test part succeeded, we don't need to do anything.
        case testing::TestPartResult::kSuccess:
            return;
        
        case testing::TestPartResult::kSkip:
            Er::Log2::info(Er::Log2::get(), "{}({}) Skipped", result.file_name(), result.line_number());

        case testing::TestPartResult::kNonFatalFailure:
        case testing::TestPartResult::kFatalFailure:
            Er::Log2::info(Er::Log2::get(), "{}({}): error:", result.file_name(), result.line_number());
            if (result.message() && *result.message())
                Er::Log2::info(Er::Log2::get(), "{}", result.message());
        }
    }
    
    void OnTestEnd(const testing::TestInfo& test) override 
    {
        if (test.result()->Passed()) 
        {
            Er::Log2::info(Er::Log2::get(), "[       OK ] {}.{} ({} ms)", test.test_suite_name(), test.name(), test.result()->elapsed_time());
        }
        else if (test.result()->Skipped()) 
        {
            Er::Log2::info(Er::Log2::get(), "[  SKIPPED ] {}.{} ({} ms)", test.test_suite_name(), test.name(), test.result()->elapsed_time());
        }
        else 
        {
            Er::Log2::info(Er::Log2::get(), "[  FAILED  ] {}.{} ({} ms)", test.test_suite_name(), test.name(), test.result()->elapsed_time());
        }
        
        if (test.result()->Failed())
        {
            auto type_param = test.type_param();
            auto value_param = test.value_param();

            if (type_param) 
            {
                Er::Log2::info(Er::Log2::get(), "TypeParam = {}", type_param);
            }

            if (value_param) 
            {
                Er::Log2::info(Er::Log2::get(), "GetParam() = {}", value_param);
            }
        }
    }
};



class TestApplication final
    : public Er::Program
{
public:
    TestApplication(int options) noexcept
        : Er::Program(options)
    {
    }

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

        {
            auto sink = std::make_shared<CapturedStderr>();
            main->addSink("capture", sink);
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
#if 1
        auto& listeners = testing::UnitTest::GetInstance()->listeners();
        delete listeners.Release(listeners.default_result_printer());
        listeners.Append(new TestEventListener);
#endif
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

        TestApplication app{ Er::Program::Options::SyncLogger };

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

