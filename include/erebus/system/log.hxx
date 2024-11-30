#pragma once

#include <erebus/system/format.hxx>
#include <erebus/system/system/packed_time.hxx>
#include <erebus/system/system/thread.hxx>

#include <boost/noncopyable.hpp>

namespace Er::Log
{

enum class Level
{
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
    Off
};


struct Record
{
public:
    using Ptr = std::shared_ptr<Record>;

    [[nodiscard]] constexpr Level level() const noexcept
    {
        return m_level;
    }

    [[nodiscard]] constexpr System::PackedTime::ValueType time() const noexcept
    {
        return m_time;
    }

    [[nodiscard]] constexpr uintptr_t tid() const noexcept
    {
        return m_tid;
    }

    [[nodiscard]] constexpr const std::string& message() const noexcept
    {
        return m_message;
    }

    [[nodiscard]] constexpr unsigned indent() const noexcept
    {
        return m_indent;
    }

    void setIndent(unsigned indent) noexcept
    {
        // use in the top level logger only
        // since the structure may be shared between many consumers
        m_indent = indent;
    }

    template <typename MessageT>
    [[nodiscard]] static Ptr make(Level level, System::PackedTime::ValueType time, uintptr_t tid, MessageT&& message)
    {
        return std::shared_ptr<Record>(new Record(level, time, tid, std::forward<MessageT>(message)));
    }

private:
    template <typename MessageT>
    explicit Record(Level level, System::PackedTime::ValueType time, uintptr_t tid, MessageT&& message)
        : m_level(level)
        , m_time(time)
        , m_tid(tid)
        , m_message(std::forward<MessageT>(message))
    {
    }

    Level m_level = Level::Info;
    System::PackedTime::ValueType m_time;
    uintptr_t m_tid = 0;
    std::string m_message;
    unsigned m_indent = 0;
};


struct ISink
{
    using Ptr = std::shared_ptr<ISink>;

    virtual ~ISink() = default;
    
    constexpr ISink() noexcept = default;
    
    constexpr [[nodiscard]] Level level() const noexcept
    {
        return m_level;
    }

    Level setLevel(Level level)
    {
        auto prev = m_level;
        m_level = level;
        doSetLevel(level);
        return prev;
    }

    virtual void write(Record::Ptr r) = 0;
    virtual void flush() = 0;
    
protected:
    virtual void doSetLevel(Level) { }

    Level m_level = Level::Debug;
};


struct ITee
    : public ISink
{
    using Ptr = std::shared_ptr<ITee>;

    virtual void addSink(std::string_view name, ISink::Ptr sink) = 0;
    virtual void removeSink(std::string_view name) = 0;
};


struct ILog
    : public ITee
{
    using Ptr = std::shared_ptr<ILog>;

    virtual void indent() = 0;
    virtual void unindent() = 0;
};


struct Indent
    : public boost::noncopyable
{
    ~Indent()
    {
        log->unindent();
    }

    Indent(ILog* log)
        : log(log)
    {
        ErAssert(log);
        log->indent();
    }

private:
    ILog* log;
};


inline void writeln(ISink* sink, Level level, const std::string& text)
{
    sink->write(Record::make(
        level,
        System::PackedTime::now(),
        System::CurrentThread::id(),
        text
    ));
}

inline void writeln(ISink* sink, Level level, std::string&& text)
{
    sink->write(Record::make(
        level,
        System::PackedTime::now(),
        System::CurrentThread::id(),
        std::move(text)
    ));
}

template <class... Args>
void write(ISink* sink, Level level, std::string_view format, Args&&... args)
{
    sink->write(Record::make(
        level, 
        System::PackedTime::now(), 
        System::CurrentThread::id(), 
        Format::vformat(format, Format::make_format_args(args...))
    ));
}

template <class... Args>
void debug(ISink* sink, std::string_view format, Args&&... args)
{
    if (sink->level() <= Level::Debug)
        write(sink, Level::Debug, format, std::forward<Args>(args)...);
}

template <class... Args>
void info(ISink* sink, std::string_view format, Args&&... args)
{
    if (sink->level() <= Level::Info)
        write(sink, Level::Info, format, std::forward<Args>(args)...);
}

template <class... Args>
void warning(ISink* sink, std::string_view format, Args&&... args)
{
    if (sink->level() <= Level::Warning)
        write(sink, Level::Warning, format, std::forward<Args>(args)...);
}

template <class... Args>
void error(ISink* sink, std::string_view format, Args&&... args)
{
    if (sink->level() <= Level::Error)
    {
        write(sink, Level::Error, format, std::forward<Args>(args)...);
        sink->flush();
    }
}

template <class... Args>
void fatal(ISink* sink, std::string_view format, Args&&... args)
{
    if (sink->level() <= Level::Fatal)
    {
        write(sink, Level::Fatal, format, std::forward<Args>(args)...);
        sink->flush();
    }
}


enum class ThreadSafe
{
    No,
    Yes
};


struct IFormatter
{
    using Ptr = std::shared_ptr<IFormatter>;

    virtual ~IFormatter() = default;
    virtual [[nodiscard]] std::string format(const Record* r) const = 0;
};


struct IFilter
{
    using Ptr = std::shared_ptr<IFilter>;

    virtual ~IFilter() = default;
    virtual [[nodiscard]] bool filter(const Record* r) const = 0;
};


struct SimpleFilter
    : public IFilter
{
    ~SimpleFilter() = default;

    [[nodiscard]] bool filter(const Record* r) const override
    {
        return (r->level() >= m_lowest) && (r->level() <= m_highest);
    }

    static [[nodiscard]] Ptr make(Level lowest, Level highest)
    {
        return std::shared_ptr<SimpleFilter>(new SimpleFilter(lowest, highest));
    }

private:
    SimpleFilter(Level lowest, Level highest)
        : m_lowest(lowest)
        , m_highest(highest)
    {
    }

    Level m_lowest;
    Level m_highest;
};

//
// per-thread default logger
//

ER_SYSTEM_EXPORT [[nodiscard]] ILog* get() noexcept;
ER_SYSTEM_EXPORT ILog* set(ILog* log) noexcept;


ER_SYSTEM_EXPORT [[nodiscard]] ILog::Ptr makeAsyncLogger();

ER_SYSTEM_EXPORT [[nodiscard]] ISink::Ptr makeFileSink(
    ThreadSafe mode, 
    std::string_view fileName,
    IFormatter::Ptr formatter,
    IFilter::Ptr filter = IFilter::Ptr{},
    unsigned logsToKeep = 3, 
    std::uint64_t maxFileSize = (std::numeric_limits<std::uint64_t>::max)()
);


ER_SYSTEM_EXPORT [[nodiscard]] ISink::Ptr makeOStreamSink(std::ostream& stream, IFormatter::Ptr formatter, IFilter::Ptr filter = IFilter::Ptr{});

#if ER_WINDOWS
ER_SYSTEM_EXPORT [[nodiscard]] ISink::Ptr makeDebuggerSink(IFormatter::Ptr formatter, IFilter::Ptr filter = IFilter::Ptr{});
#endif


} // namespace Er::Log {}

#include <erebus/system/logger/simple_formatter.hxx>


