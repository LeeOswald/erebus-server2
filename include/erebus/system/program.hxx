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
    enum 
    {
        EnableSignalHandler = 0x0001,
        CanBeDaemonized = 0x0002
    };

    static void globalStartup(int argc, char** argv, unsigned options = 0) noexcept;
    static void globalShutdown() noexcept;

    virtual ~Program();
    Program() noexcept;

    static bool optionPresent(int argc, char** argv, const char* longName, const char* shortName) noexcept;

    static Program* instance() noexcept
    {
        return s_instance.load(std::memory_order_acquire);
    }

    static bool isDaemon() noexcept
    {
        return s_isDaemon;
    }

    static constexpr Waitable<bool>& exitCondition() noexcept
    {
        return s_exitCondition;
    }

    static int signalReceived() noexcept
    {
        return s_signalReceived.load(std::memory_order_acquire);
    }

    constexpr const boost::program_options::variables_map& options() const noexcept
    {
        return m_options;
    }

    bool verbose() const noexcept;

    int exec(int argc, char** argv) noexcept;

protected:
    virtual void terminateHandler();
    virtual void signalHandler(int signo);
    virtual void printAssertFn(std::string_view message);
    virtual void addCmdLineOptions(boost::program_options::options_description& options);
    virtual void displayHelp(const boost::program_options::options_description& options);
    virtual bool loadConfiguration();
    virtual int run() = 0;

private:
    static void staticTerminateHandler();
    static void defaultTerminateHandler();
    static void staticSignalHandler(int signo);
    static void staticPrintAssertFn(std::string_view message);
    static void defaultPrintAssertFn(std::string_view message);
    bool doLoadConfiguration(int argc, char** argv);
    
    static unsigned s_options;
    static std::atomic<Program*> s_instance;
    static bool s_isDaemon;
    static Waitable<bool> s_exitCondition;
    static std::atomic<int> s_signalReceived;

#if ER_POSIX
    struct SignalWaiter
    {
        System::SignalHandler sh;
        std::future<int> fu;

        SignalWaiter(const std::initializer_list<int>& signals)
            : sh(signals)
            , fu(sh.asyncWaitHandler(
                [](int signo)
                {
                    staticSignalHandler(signo);
                    return true;
                }))
        {}
    };

    static std::unique_ptr<SignalWaiter> s_signalWaiter;
#endif

    boost::program_options::variables_map m_options;
    Log2::ILogger::Ptr m_logger;
};


} // namespace Er {}