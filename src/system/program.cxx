#include <erebus/system/program.hxx>
#include <erebus/system/system/process.hxx>
#include <erebus/system/util/exception_util.hxx>

#if ER_WINDOWS
    #include <erebus/system/logger/win32_debugger_sink2.hxx>
#endif

#if ER_LINUX
    #include <erebus/system/logger/syslog_linux_sink2.hxx>
#endif

#include <erebus/system/logger/ostream_sink2.hxx>
#include <erebus/system/logger/simple_formatter2.hxx>
#include <erebus/system/type_id.hxx>

#include <boost/stacktrace.hpp>

#include <csignal>
#include <filesystem>
#include <iostream>



namespace Er
{

Program* Program::s_instance = nullptr;


Program::~Program()
{
    Erp::Log2::setGlobal({});
    s_instance = nullptr;
}

Program::Program(int options) noexcept
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

    Log2::fatal(Er::Log2::get(), "std::terminate() called from\n{}", ss.str());
    Er::Log2::get()->flush();

    std::abort();
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
    Log2::writeln(Er::Log2::get(), Log2::Level::Fatal, std::string(message));
    Er::Log2::get()->flush();
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
#if ER_WINDOWS
        m_signalWaiter.reset(new SignalWaiter(this, { SIGINT, SIGTERM }));
#else
        m_signalWaiter.reset(new SignalWaiter(this, { SIGINT, SIGTERM, SIGPIPE, SIGHUP }));
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
    if (m_signalWaiter)
    {
#if ER_POSIX
        // m_signalWaiter is locked in sigwait() so wake it 
        ::kill(::getpid(), SIGHUP);
#endif
        m_signalWaiter.reset();
    }

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
        auto sink = Log2::makeDebuggerSink(
            Log2::SimpleFormatter::make(),
            Log2::Filter{}
        );

        main->addSink("debugger", sink);
    }
#endif

#if ER_LINUX
    if (m_isDaemon)
    {
        auto sink = Log2::makeSyslogSink("erebus", Log2::SimpleFormatter::make(
                Log2::SimpleFormatter::Options{ 
                Log2::SimpleFormatter::Option::Time, 
                Log2::SimpleFormatter::Option::Level, 
                Log2::SimpleFormatter::Option::Tid, 
                Log2::SimpleFormatter::Option::TzLocal, 
                Log2::SimpleFormatter::Option::Lf}),
                [](const Log2::Record* r) { return r->level() >= Log2::Level::Error; });

        main->addSink("syslog", sink);
    }
#endif

    if (!m_isDaemon)
    {
        auto sink = makeOStreamSink(
            std::cout,
            Log2::SimpleFormatter::make(),
            [](const Log2::Record* r)
            {
                return r->level() < Er::Log2::Level::Error;
            }
        );

        main->addSink("std::cout", sink);
    }

    if (!m_isDaemon)
    {
        auto sink = makeOStreamSink(
            std::cerr,
            Log2::SimpleFormatter::make(),
            [](const Log2::Record* r)
            {
                return r->level() >= Er::Log2::Level::Error;
            }
        );

        main->addSink("std::cerr", sink);
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
    if (m_options & SyncLogger)
        m_logger = Log2::makeSyncLogger();
    else
        m_logger = Log2::makeLogger({}, std::chrono::milliseconds(m_loggerThreshold));

    auto verbose = m_args.count("verbose") > 0;
    m_logger->setLevel(verbose ? Log2::Level::Debug : Log2::Level::Info);

    Er::Log2::g_verbose = verbose;

    if (m_options & SyncLogger)
    {
        auto tee = Log2::makeTee(ThreadSafe::Yes);
        m_logger->addSink("tee", std::static_pointer_cast<Log2::ISink>(tee));

        addLoggers(tee.get());
    }
    else
    {
        auto tee = Log2::makeTee(ThreadSafe::No); // tee is called from the single logger thread
        m_logger->addSink("tee", std::static_pointer_cast<Log2::ISink>(tee));

        addLoggers(tee.get());
    }

    Erp::Log2::setGlobal(m_logger);
}

int Program::exec(int argc, char** argv)
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
        initializeTypeRegistry(m_logger.get());
        run(argc, argv);
        finalizeTypeRegistry();
    }
    catch(...)
    {
        dispatchException(std::current_exception(), xcptLogger);
    }

    globalShutdown();

    return resut;
}


} // namespace Er {}