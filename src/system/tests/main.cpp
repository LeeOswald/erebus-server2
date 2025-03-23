#include <erebus/testing/test_application.hxx>
#include <erebus/testing/test_event_listener.hxx>

#include "common.hpp"

int InstanceCounter::instances = 0;

class App
    : public Erp::Testing::TestApplication
{
public:
    using Base = Erp::Testing::TestApplication;

    App()
        : Base(Er::Program::Options::SyncLogger)
    {
    }

private:
    void addLoggers(Er::Log2::ITee* main) override
    {
        Base::addLoggers(main);

        {
            auto sink = std::make_shared<CapturedStderr>();
            main->addSink("capture", sink);
        }
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

