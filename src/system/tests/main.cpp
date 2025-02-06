#include "common.hpp"

#include <erebus/system/program.hxx>


int InstanceCounter::instances = 0;

namespace
{

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

