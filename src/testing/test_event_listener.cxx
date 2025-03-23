#include <erebus/testing/test_event_listener.hxx>

#include <erebus/system/logger2.hxx>

namespace Erp::Testing
{


void TestEventListener::OnTestIterationStart(const testing::UnitTest& test, int iteration)
{
    Er::Log2::info(Er::Log2::get(), "[==========] Running {} from {}.", test.test_to_run_count(), test.test_suite_to_run_count());
}

void TestEventListener::OnTestIterationEnd(const testing::UnitTest& test, int iteration)
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

void TestEventListener::OnEnvironmentsSetUpStart(const testing::UnitTest&)
{
    Er::Log2::info(Er::Log2::get(), "[----------] Global test environment set-up.");
}

void TestEventListener::OnEnvironmentsTearDownStart(const testing::UnitTest&)
{
    Er::Log2::info(Er::Log2::get(), "[----------] Global test environment tear-down");
}

void TestEventListener::OnTestSuiteStart(const testing::TestSuite& suite)
{
    Er::Log2::info(Er::Log2::get(), "[----------] {} from {}", suite.test_to_run_count(), suite.name());
}

void TestEventListener::OnTestSuiteEnd(const testing::TestSuite& suite)
{
    Er::Log2::info(Er::Log2::get(), "[----------] {} from {} ({} ms total)\n", suite.test_to_run_count(), suite.name(), suite.elapsed_time());
}

void TestEventListener::OnTestStart(const testing::TestInfo& test)
{
    Er::Log2::info(Er::Log2::get(), "[ RUN      ] {}.{}", test.test_suite_name(), test.name());
}

void TestEventListener::OnTestDisabled(const testing::TestInfo& test)
{
    Er::Log2::info(Er::Log2::get(), "[ DISABLED ] {}.{}", test.test_suite_name(), test.name());
}

void TestEventListener::OnTestPartResult(const testing::TestPartResult& result)
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

void TestEventListener::OnTestEnd(const testing::TestInfo& test)
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

} // namespace Erp::Testing {}