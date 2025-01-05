#include <erebus/system/logger/win32_debugger_sink2.hxx>
#include <erebus/system/util/utf16.hxx>


namespace Er::Log2
{

namespace
{

class DebuggerSink
    : public SinkBase
{
public:
    DebuggerSink(IFormatter::Ptr formatter, Filter&& filter)
        : SinkBase(formatter, std::move(filter))
    {
    }

    void write(Record::Ptr r) override
    {
        if (!::IsDebuggerPresent())
            return;

        if (!filter(r.get()))
            return;

        auto formatted = format(r.get());
        if (formatted.empty())
            return;

        auto u16 = Util::utf8ToUtf16(formatted);
        ::OutputDebugStringW(u16.c_str());
    }

    void flush() override
    {
    }
};

} // namespace {}


ER_SYSTEM_EXPORT ISink::Ptr makeDebuggerSink(IFormatter::Ptr formatter, Filter&& filter)
{
    return std::make_shared<DebuggerSink>(formatter, std::move(filter));
}

} // namespace Er::Log2 {}