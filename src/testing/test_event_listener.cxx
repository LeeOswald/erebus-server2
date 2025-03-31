#include <erebus/testing/test_event_listener.hxx>

#include <erebus/system/logger2.hxx>

namespace Erp::Testing
{


void TestEventListener::OnTestIterationStart(const testing::UnitTest& test, int iteration)
{
    ErLogInfo("[==========] Running {} from {}.", test.test_to_run_count(), test.test_suite_to_run_count());
}

void TestEventListener::OnTestIterationEnd(const testing::UnitTest& test, int iteration)
{
    ErLogInfo("[==========] {} from {} ran ({} ms total).", test.test_to_run_count(), test.test_suite_to_run_count(), test.elapsed_time());

    ErLogInfo("[  PASSED  ] {}", test.successful_test_count());

    auto skipped_test_count = test.skipped_test_count();
    if (skipped_test_count > 0)
    {
        ErLogInfo("[  SKIPPED ] {}", skipped_test_count);
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
                ErLogInfo("[  SKIPPED ] {}.{}", test_suite.name(), test_info.name());
            }
        }
    }

    if (!test.Passed())
    {
        auto failed_test_count = test.failed_test_count();
        ErLogInfo("[  FAILED  ] {}", failed_test_count);

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
                ErLogInfo("[  FAILED  ] {}.{}", test_suite.name(), test_info.name());

                auto type_param = test_info.type_param();
                auto value_param = test_info.value_param();

                if (type_param)
                {
                    ErLogInfo("TypeParam = {}", type_param);
                }

                if (value_param)
                {
                    ErLogInfo("GetParam() = {}", value_param);
                }
            }
        }
        ErLogInfo("{} FAILED TESTS", failed_test_count);
    }

    auto num_disabled = test.reportable_disabled_test_count();
    if (num_disabled)
    {
        if (test.Passed())
        {
            ErLogInfo("");  // Add a spacer if no FAILURE banner is displayed.
        }

        ErLogInfo("  YOU HAVE {} DISABLED TESTS\n", num_disabled);
    }
}

void TestEventListener::OnEnvironmentsSetUpStart(const testing::UnitTest&)
{
    ErLogInfo("[----------] Global test environment set-up.");
}

void TestEventListener::OnEnvironmentsTearDownStart(const testing::UnitTest&)
{
    ErLogInfo("[----------] Global test environment tear-down");
}

void TestEventListener::OnTestSuiteStart(const testing::TestSuite& suite)
{
    ErLogInfo("[----------] {} from {}", suite.test_to_run_count(), suite.name());
}

void TestEventListener::OnTestSuiteEnd(const testing::TestSuite& suite)
{
    ErLogInfo("[----------] {} from {} ({} ms total)\n", suite.test_to_run_count(), suite.name(), suite.elapsed_time());
}

void TestEventListener::OnTestStart(const testing::TestInfo& test)
{
    ErLogInfo("[ RUN      ] {}.{}", test.test_suite_name(), test.name());
}

void TestEventListener::OnTestDisabled(const testing::TestInfo& test)
{
    ErLogInfo("[ DISABLED ] {}.{}", test.test_suite_name(), test.name());
}

void TestEventListener::OnTestPartResult(const testing::TestPartResult& result)
{
    switch (result.type())
    {
        // If the test part succeeded, we don't need to do anything.
    case testing::TestPartResult::kSuccess:
        return;

    case testing::TestPartResult::kSkip:
        ErLogInfo("{}({}) Skipped", result.file_name(), result.line_number());

    case testing::TestPartResult::kNonFatalFailure:
    case testing::TestPartResult::kFatalFailure:
        ErLogInfo("{}({}): error:", result.file_name(), result.line_number());
        if (result.message() && *result.message())
            ErLogInfo("{}", result.message());
    }
}

void TestEventListener::OnTestEnd(const testing::TestInfo& test)
{
    if (test.result()->Passed())
    {
        ErLogInfo("[       OK ] {}.{} ({} ms)", test.test_suite_name(), test.name(), test.result()->elapsed_time());
    }
    else if (test.result()->Skipped())
    {
        ErLogInfo("[  SKIPPED ] {}.{} ({} ms)", test.test_suite_name(), test.name(), test.result()->elapsed_time());
    }
    else
    {
        ErLogInfo("[  FAILED  ] {}.{} ({} ms)", test.test_suite_name(), test.name(), test.result()->elapsed_time());
    }

    if (test.result()->Failed())
    {
        auto type_param = test.type_param();
        auto value_param = test.value_param();

        if (type_param)
        {
            ErLogInfo("TypeParam = {}", type_param);
        }

        if (value_param)
        {
            ErLogInfo("GetParam() = {}", value_param);
        }
    }
}

} // namespace Erp::Testing {}