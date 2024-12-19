#include <erebus/system/program.hxx>
#include <erebus/system/system/process.hxx>

#include <boost/stacktrace.hpp>

#include <csignal>
#include <filesystem>
#include <iostream>



namespace Er
{

unsigned Program::s_options = 0;
std::atomic<Program*> Program::s_instance = nullptr;
bool Program::s_isDaemon = false;
Waitable<bool> Program::s_exitCondition{false};
std::atomic<int> Program::s_signalReceived = 0;

bool Program::optionPresent(int argc, char** argv, const char* longName, const char* shortName) noexcept
{
    for (int i = 1; i < argc; ++i)
    {
        if (longName && !std::strcmp(argv[i], longName))
            return true;

        if (shortName && !std::strcmp(argv[i], shortName))
            return true;
    }

    return false;
}

bool Program::verbose() const noexcept
{
    return m_options.count("verbose") > 0;
}

void Program::staticTerminateHandler()
{
    auto this_ = instance();
    if (this_)
        this_->terminateHandler();
    else
        defaultTerminateHandler();
}

void Program::defaultTerminateHandler()
{
    std::ostringstream ss;
    ss << boost::stacktrace::stacktrace();

    Log::fatal(Log::get(), "std::terminate() called from\n{}", ss.str());

    std::abort();
}

void Program::terminateHandler()
{
    defaultTerminateHandler();
}

void Program::staticSignalHandler(int signo)
{
    auto this_ = instance();
    if (this_)
        this_->signalHandler(signo);

    s_signalReceived = signo;
    s_exitCondition.setAndNotifyAll(true);
}

void Program::signalHandler(int signo)
{
}

void Program::staticPrintAssertFn(std::string_view message)
{
    auto this_ = instance();
    if (this_)
        this_->printAssertFn(message);
    else
        defaultPrintAssertFn(message);
}

void Program::defaultPrintAssertFn(std::string_view message)
{
    Log::writeln(Log::get(), Log::Level::Fatal, std::string(message));
}

void Program::printAssertFn(std::string_view message)
{
    defaultPrintAssertFn(message);
}

void Program::globalStartup(int argc, char** argv, unsigned options) noexcept
{
    s_options = options;

#if ER_POSIX
    // daemonize as early as possible to avoid a whole bunch of bugs
    if (options & CanBeDaemonized)
    {
        bool daemonize = optionPresent(argc, argv, "--daemon", "-d");
        if (daemonize)
        {
            Er::System::CurrentProcess::daemonize();
            s_isDaemon = true;
        }
    }
#endif

#if ER_WINDOWS
    ::SetConsoleOutputCP(CP_UTF8);
#endif

#if ER_DEBUG && defined(_MSC_VER)
    // use MSVC debug heap
    int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(tmpFlag);
#endif

    if (options & EnableSignalHandler)
    {
#if ER_POSIX
        s_signalWaiter.reset(new SignalWaiter{ SIGINT, SIGTERM, SIGPIPE, SIGHUP });
#else
        ::signal(SIGINT, staticSignalHandler);
        ::signal(SIGTERM, staticSignalHandler);
#endif
    }
    
    std::set_terminate(staticTerminateHandler);

    setPrintFailedAssertionFn(staticPrintAssertFn);

    // set current dir the same as exe dir
    {
        std::filesystem::path exe(Er::System::CurrentProcess::exe());
        auto dir = exe.parent_path();
        std::error_code ec;
        std::filesystem::current_path(dir, ec);
    }
}

void Program::globalShutdown() noexcept
{
#if ER_POSIX
    if(s_signalWaiter)
    {
        // s_signalWaiter is locked in sigwait() so wake it 
        ::kill(::getpid(), SIGHUP);
        s_signalWaiter.reset();
    }
#endif

    Er::setPrintFailedAssertionFn(nullptr);
}

Program::~Program()
{
    s_instance = nullptr;
}

Program::Program() noexcept
{
    // no complex initialization here
    ErAssert(!s_instance);
    s_instance = this;
}

void Program::addCmdLineOptions(boost::program_options::options_description& options)
{
}

void Program::displayHelp(const boost::program_options::options_description& options)
{
    std::cout << options << std::endl;
}

bool Program::loadConfiguration()
{
    return true;
}

bool Program::doLoadConfiguration(int argc, char** argv)
{
    boost::program_options::options_description options("Command line options");
    options.add_options()
        ("help,?", "show help")
        ("verbose,v", "verbose logging");


#if ER_POSIX
    if (s_options & CanBeDaemonized)
        options.add_options()
            ("daemon,d", "run as a daemon");
#endif

    addCmdLineOptions(options);

    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, options), m_options);
    boost::program_options::notify(m_options);

    if (m_options.count("help"))
    {
        displayHelp(options);
        return false;
    }

    return loadConfiguration();
}

int Program::exec(int argc, char** argv) noexcept
{
    try
    {

    }
    catch(...)
    {
        
    }
}


} // namespace Er {}