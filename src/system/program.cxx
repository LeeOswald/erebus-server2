#include <erebus/system/program.hxx>
#include <erebus/system/system/process.hxx>
#include <erebus/system/util/exception_util.hxx>

#if ER_WINDOWS
    #include <erebus/system/logger/win32_debugger_sink2.hxx>
#endif

#if ER_LINUX
    #include <erebus/system/logger/syslog_linux_sink2.hxx
#endif

#include <erebus/system/logger/ostream_sink2.hxx>
#include <erebus/system/logger/simple_formatter2.hxx>

#include <boost/stacktrace.hpp>

#include <csignal>
#include <filesystem>
#include <iostream>



namespace Er
{

Program* Program::s_instance = nullptr;


Program::~Program()
{
    s_instance = nullptr;
}

Program::Program(unsigned options) noexcept
    : m_options(options)
{
    // no complex initialization here
    s_instance = this;
}

bool Program::argPresent(int argc, char** argv, const char* longName, const char* shortName) noexcept
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

void Program::staticTerminateHandler()
{
    if (s_instance)
        s_instance->terminateHandler();
    else
        std::abort();
}

void Program::terminateHandler()
{
    std::ostringstream ss;
    ss << boost::stacktrace::stacktrace();

    Log2::fatal(Log2::get(), "std::terminate() called from\n{}", ss.str());

    std::abort();
}

void Program::staticSignalHandler(int signo)
{
    if (s_instance)
        s_instance->signalHandler(signo);
}

void Program::signalHandler(int signo)
{
    m_signalReceived = signo;
    m_exitCondition.setAndNotifyAll(true);
}

void Program::staticPrintAssertFn(std::string_view message)
{
    if (s_instance)
        s_instance->printAssertFn(message);
}

void Program::printAssertFn(std::string_view message)
{
    Log2::writeln(Log2::get(), Log2::Level::Fatal, std::string(message));
}

void Program::globalStartup(int argc, char** argv) noexcept
{
#if ER_POSIX
    // daemonize as early as possible to avoid a whole bunch of bugs
    if (m_options & CanBeDaemonized)
    {
        bool daemonize = argPresent(argc, argv, "--daemon", "-d");
        if (daemonize)
        {
            Er::System::CurrentProcess::daemonize();
            m_isDaemon = true;
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

    // setup signal handler
    if (m_options & EnableSignalHandler)
    {
#if ER_POSIX
        m_signalWaiter.reset(new SignalWaiter({ SIGINT, SIGTERM, SIGPIPE, SIGHUP }, this));
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
    if (m_signalWaiter)
    {
        // s_signalWaiter is locked in sigwait() so wake it 
        ::kill(::getpid(), SIGHUP);
        m_signalWaiter.reset();
    }
#endif

    Er::setPrintFailedAssertionFn(nullptr);
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

void Program::addLoggers(Log2::ITee* main)
{
#if ER_WINDOWS
    if (isDebuggerPresent())
    {
        Log2::makeDebuggerSink(
            Log2::SimpleFormatter::make(),
            Log2::Filter{}
        );
    }
#endif

#if ER_LINUX
    if (m_isDaemon)
    {

    }
#endif

    if (!m_isDaemon)
    {
        makeOStreamSink(
            std::cout,
            Log2::SimpleFormatter::make(),
            [](const Log2::Record* r)
            {
                return r->level() < Er::Log2::Level::Error;
            }
        );
    }

    if (!m_isDaemon)
    {
        makeOStreamSink(
            std::cerr,
            Log2::SimpleFormatter::make(),
            [](const Log2::Record* r)
            {
                return r->level() >= Er::Log2::Level::Error;
            }
        );
    }
}

bool Program::globalLoadConfiguration(int argc, char** argv)
{
    boost::program_options::options_description options("Command line options");
    options.add_options()
        ("help,?", "show help")
        ("verbose,v", "verbose logging")
        ("logthreshold", boost::program_options::value<unsigned>(&m_loggerThreshold)->default_value(unsigned(1000)), "lt");


#if ER_POSIX
    if (m_options & CanBeDaemonized)
        options.add_options()
            ("daemon,d", "run as a daemon");
#endif

    addCmdLineOptions(options);

    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, options), m_args);
    boost::program_options::notify(m_args);

    if (m_args.count("help"))
    {
        displayHelp(options);
        return false;
    }

    return loadConfiguration();
}

void Program::globalMakeLogger()
{
    m_logger = Log2::makeLogger(std::chrono::milliseconds(m_loggerThreshold));
    auto verbose = m_args.count("verbose") > 0;
    m_logger->setLevel(verbose ? Log2::Level::Debug : Log2::Level::Info);

    auto tee = Log2::makeTee(ThreadSafe::Yes); // tee is called from the single logger thread
    m_logger->addSink("tee", std::static_pointer_cast<Log2::ISink>(tee));

    addLoggers(tee.get());
}

int Program::exec(int argc, char** argv) noexcept
{
    int resut = EXIT_FAILURE;
    globalStartup(argc, argv);
    
    if (globalLoadConfiguration(argc, argv))
    {
        globalMakeLogger();
    }

    Util::ExceptionLogger xcptLogger(m_logger.get());

    try
    {
        run();
    }
    catch(...)
    {
        dispatchException(std::current_exception(), xcptLogger);
    }

    globalShutdown();

    return resut;
}


} // namespace Er {}