#pragma once

#if !defined(ER_WINDOWS) && !defined(ER_LINUX)
    #error Unknown OS. Check your build configuration.
#endif


#if !defined(ER_64) && !defined(ER_32)
    #error Unknown platform bitness. Check your build configuration.
#endif

#if !defined(NDEBUG)
    #define ER_DEBUG 1
#else
    #define ER_DEBUG 0
#endif

#if ER_WINDOWS
    #ifdef ER_SYSTEM_EXPORTS
        #define ER_SYSTEM_EXPORT __declspec(dllexport)
    #else
        #define ER_SYSTEM_EXPORT __declspec(dllimport)
    #endif
#else
    #define ER_SYSTEM_EXPORT __attribute__((visibility("default")))
#endif

#define EREBUS_HXX_INCLUDED 1

// absolutely necessary system headers go here
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>


namespace Er
{

namespace Log
{
    struct ILog;
}

ER_SYSTEM_EXPORT void initialize(Er::Log::ILog* log);
ER_SYSTEM_EXPORT void finalize(Er::Log::ILog* log) noexcept;


} // namespace Er {}


// all mush-have local stuff goes here
#include <erebus/system/assert.hxx>

// use std min/max
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif