#pragma once

#include <erebus/system/erebus.hxx>

#include <fmt/format.h>


namespace Er
{

namespace Format = ::fmt;

template <class... Args>
std::string format(std::string_view formatString, Args&&... args) 
{
    return Format::vformat(formatString, Format::make_format_args(args...));
}


} // namespace Er {}
