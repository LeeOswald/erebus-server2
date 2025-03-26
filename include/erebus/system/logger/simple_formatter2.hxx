#pragma once

#include <erebus/system/flags.hxx>
#include <erebus/system/logger2.hxx>

#include <iomanip>
#include <sstream>


namespace Er::Log2
{

struct ER_SYSTEM_EXPORT SimpleFormatter
{
public:
    struct Option
    {
        static constexpr Flag DateTime = 0;   
        static constexpr Flag Time = 1;
        static constexpr Flag Level = 2;
        static constexpr Flag Tid = 3;
        static constexpr Flag TzUtc = 4;
        static constexpr Flag TzLocal = 5;
        static constexpr Flag Lf = 6;
        static constexpr Flag CrLf = 7;
        static constexpr Flag NoLf = 8;       // neiter CR nor LF
        static constexpr Flag Component = 9;
    };

    using Options = FlagsPack<32, Option>;

    static IFormatter::Ptr make(
        Options options = Options{ Option::Time, Option::Level, Option::Tid, Option::TzLocal, Option::Lf, Option::Component }, 
        unsigned indentSize = 4
        );
};


} // namespace Er::Log2 {}