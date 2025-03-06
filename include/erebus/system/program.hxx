#pragma once

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/program_options.hpp>

#include <erebus/system/logger2.hxx>
#include <erebus/system/waitable.hxx>

#include <atomic>
#include <thread>


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
    static void staticPrintAssertFn(std::string_view message);

    void globalStartup(int argc, char** argv) noexcept;
    void globalShutdown() noexcept;
    bool globalLoadConfiguration(int argc, char** argv);
    void globalMakeLogger();

    static Program* s_instance;
    
    int m_options;
    bool m_isDaemon = false;
    Waitable<bool> m_exitCondition;
    std::atomic<int> m_signalReceived;

    class SignalWaiter
    {
    public:
        ~SignalWaiter()
        {
            m_io.stop();
        }

        template <typename... Signals>
        SignalWaiter(Program* owner, Signals... signals)
            : m_owner(owner)
            , m_io()
            , m_wg(m_io.get_executor())
            , m_signals(m_io, std::forward<Signals>(signals)...)
            , m_worker([this]() { run(); })
        {}

    private:
        void run()
        {
            m_signals.async_wait([this](const boost::system::error_code& ec, int signo) { handler(ec, signo); });
            m_io.run();
        }

        void handler(const boost::system::error_code& ec, int signo)
        {
            if (!ec)
            {
                m_owner->signalHandler(signo);
            }
            else
            {
                Log2::fatal(Log2::get(), "Signal handler failed: {}", ec.to_string());
                std::abort();
            }
        }

        Program* m_owner;
        boost::asio::io_context m_io;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_wg;
        boost::asio::signal_set m_signals;
        std::jthread m_worker;
    };

    std::unique_ptr<SignalWaiter> m_signalWaiter;

    boost::program_options::variables_map m_args;
    unsigned m_loggerThreshold = 0;
    Log2::ILogger::Ptr m_logger;
};


} // namespace Er {}