#pragma once

#include <gtest/gtest.h>


namespace Erp::Testing
{

class TestEventListener
    : public testing::EmptyTestEventListener
{
public:
    void OnTestIterationStart(const testing::UnitTest& test, int iteration) override;
    void OnTestIterationEnd(const testing::UnitTest& test, int iteration) override;
    void OnEnvironmentsSetUpStart(const testing::UnitTest&) override;
    void OnEnvironmentsTearDownStart(const testing::UnitTest&) override;
    void OnTestSuiteStart(const testing::TestSuite& suite) override;
    void OnTestSuiteEnd(const testing::TestSuite& suite) override;
    void OnTestStart(const testing::TestInfo& test) override;
    void OnTestDisabled(const testing::TestInfo& test) override;
    void OnTestPartResult(const testing::TestPartResult& result) override;
    void OnTestEnd(const testing::TestInfo& test) override;
};



} // namespace Erp::Testing {}