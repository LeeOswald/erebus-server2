#include <erebus/testing/test_application.hxx>

#include "common.hpp"

std::uint16_t g_serverPort = 998;
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
            ("port", boost::program_options::value<std::uint16_t>(&g_serverPort)->default_value(998), "TCP port to use")
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

