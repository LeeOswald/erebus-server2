#pragma once

#include <erebus/system/logger2.hxx>
#if ER_POSIX
    #include <erebus/system/system/signal_handler_posix.hxx>
#endif
#include <erebus/system/waitable.hxx>

#include <atomic>
#include <future>

#include <boost/noncopyable.hpp>
#include <boost/program_options.hpp>


namespace Er
{


class ER_SYSTEM_EXPORT Program
    : public boost::noncopyable
{
public:
    enum Options
    {
        EnableSignalHandler = 0x0001,
        CanBeDaemonized = 0x0002,
        SyncLogger = 0x0004
    };

    
    virtual ~Program();
    Program(int options = 0) noexcept;

    bool isDaemon() noexcept
    {
        return m_isDaemon;
    }

    constexpr Waitable<bool>& exitCondition() noexcept
    {
        return m_exitCondition;
    }

    int signalReceived() noexcept
    {
        return m_signalReceived.load(std::memory_order_acquire);
    }

    constexpr const boost::program_options::variables_map& args() const noexcept
    {
        return m_args;
    }

    int exec(int argc, char** argv) noexcept;

protected:
    static bool argPresent(int argc, char** argv, const char* longName, const char* shortName) noexcept;
    virtual void terminateHandler();
    virtual void signalHandler(int signo);
    virtual void printAssertFn(std::string_view message);
    virtual void addCmdLineOptions(boost::program_options::options_description& options);
    virtual void displayHelp(const boost::program_options::options_description& options);
    virtual bool loadConfiguration();
    virtual void addLoggers(Log2::ITee* main);
    virtual int run(int argc, char** argv) = 0;

private:
    static void staticTerminateHandler();
    static void staticSignalHandler(int signo);
    static void staticPrintAssertFn(std::string_view message);

    void globalStartup(int argc, char** argv) noexcept;
    void globalShutdown() noexcept;
    bool globalLoadConfiguration(int argc, char** argv);
    void globalMakeLogger();

    static Program* s_instance;
    
    int m_options;
    bool m_isDaemon;
    Waitable<bool> m_exitCondition;
    std::atomic<int> m_signalReceived;

#if ER_POSIX
    struct SignalWaiter
    {
        System::SignalHandler sh;
        std::future<int> fu;

        SignalWaiter(const std::initializer_list<int>& signals, Program* program)
            : sh(signals)
            , fu(sh.asyncWaitHandler(
                [program](int signo)
                {
                    program->signalHandler(signo);
                    return true;
                }))
        {}
    };

    std::unique_ptr<SignalWaiter> m_signalWaiter;
#endif

    boost::program_options::variables_map m_args;
    unsigned m_loggerThreshold = 0;
    Log2::ILogger::Ptr m_logger;
};


} // namespace Er {}