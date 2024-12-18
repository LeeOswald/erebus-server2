#include <erebus/system/system/thread.hxx>
#include <erebus/system/system/signal_handler_posix.hxx>



namespace Er::System
{

SignalHandler::SignalHandler(const std::initializer_list<int>& signals) noexcept
    : m_signals()
{
    ::sigemptyset(&m_signals);

    for (auto signum : signals)
    {
        if (::sigaddset(&m_signals, signum) != 0)
        {
            ErAssert(!"Invalid signal");
        }
    }

    if (::pthread_sigmask(SIG_BLOCK, &m_signals, nullptr) != 0)
    {
        ErAssert("!Signal could not be blocked");
    }
}

SignalHandler::~SignalHandler() noexcept
{
    ::pthread_sigmask(SIG_UNBLOCK, &m_signals, nullptr);
}

int SignalHandler::wait() const
{
    int signum = 0;
    int ret = ::sigwait(&m_signals, &signum);
    ErAssert(ret == 0);

    return signum;
}

int SignalHandler::waitHandler(std::function<bool(int)> handler) const
{
    Er::System::CurrentThread::setName("SignalHandler");

    while (true)
    {
        int signum = wait();
        if (handler(signum))
            return signum;
    }
}


} // namespace Er::System {}