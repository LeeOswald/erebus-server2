#include <erebus/testing/test_application.hxx>

#include "common.hpp"

#if ER_LINUX
static constexpr std::string_view DefaultEnpoint = "unix:///tmp/erebus_grpc_test";
#else
static constexpr std::string_view DefaultEnpoint = "127.0.0.1:998";
#endif

std::string g_serverEndpoint;
std::chrono::milliseconds g_operationTimeout{ 60 * 1000 };


class App final
    : public Erp::Testing::TestApplication
{
public:
    using Base = Erp::Testing::TestApplication;

    App()
        : Base(Er::Program::Options::SyncLogger)
    {
    }

private:
    void addCmdLineOptions(boost::program_options::options_description& options) override
    {
        Base::addCmdLineOptions(options);

        options.add_options()
            ("endpoint", boost::program_options::value<std::string>(&g_serverEndpoint)->default_value(std::string(DefaultEnpoint)), "Temporary endpoint address")
            ;
    }
};


int main(int argc, char** argv)
{
    try
    {
        App app;

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

