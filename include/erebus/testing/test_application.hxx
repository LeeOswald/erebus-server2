#pragma once


#include <erebus/system/program.hxx>


namespace Erp::Testing
{

class TestApplication
    : public Er::Program
{
public:
    TestApplication(int options) noexcept;

protected:
    void addCmdLineOptions(boost::program_options::options_description& options) override;
    void addLoggers(Er::Log2::ITee* main) override;
    int run(int argc, char** argv) override;
};


} // Erp::Testing{}