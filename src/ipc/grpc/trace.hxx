#pragma once

#include <erebus/system/logger2.hxx>


#ifndef ER_GRPC_CLIENT_TRACE
    #if ER_DEBUG
        #define ER_GRPC_CLIENT_TRACE 1
    #else
        #define ER_GRPC_CLIENT_TRACE 0
    #endif
#endif


#ifndef ER_GRPC_SERVER_TRACE
    #if ER_DEBUG
        #define ER_GRPC_SERVER_TRACE 1
    #else
        #define ER_GRPC_SERVER_TRACE 0
    #endif
#endif


#if ER_GRPC_CLIENT_TRACE

#define ClientTrace(format, ...) \
    if (::Er::Log2::get()->level() <= ::Er::Log2::Level::Debug) \
        ::Er::Log2::debug(::Er::Log2::get(), format, ##__VA_ARGS__)

#define ClientTrace2(sink, format, ...) \
    if (sink->level() <= ::Er::Log2::Level::Debug) \
        ::Er::Log2::debug(sink, format, ##__VA_ARGS__)


#define ClientTraceIndent(format, ...) \
    if (::Er::Log2::get()->level() <= ::Er::Log2::Level::Debug) \
        ::Er::Log2::write(::Er::Log2::get(), ::Er::Log2::Level::Debug, format, ##__VA_ARGS__); \
    ::Er::Log2::IndentScope __ids(::Er::Log2::get(), ::Er::Log2::Level::Debug)

#define ClientTraceIndent2(sink, format, ...) \
    if (sink->level() <= ::Er::Log2::Level::Debug) \
        ::Er::Log2::write(sink, ::Er::Log2::Level::Debug, format, ##__VA_ARGS__); \
    ::Er::Log2::IndentScope __ids(sink, ::Er::Log2::Level::Debug)

#else // !ER_GRPC_CLIENT_TRACE

#define ClientTrace(format, ...)                     ((void)0)
#define ClientTrace2(sink, format, ...)              ((void)0)

#define ClientTraceIndent(format, ...)               ((void)0)
#define ClientTraceIndent2(sink, format, ...)        ((void)0)

#endif // !ER_GRPC_CLIENT_TRACE


#if ER_GRPC_SERVER_TRACE

#define ServerTrace(format, ...) \
    if (::Er::Log2::get()->level() <= ::Er::Log2::Level::Debug) \
        ::Er::Log2::debug(::Er::Log2::get(), format, ##__VA_ARGS__)

#define ServerTrace2(sink, format, ...) \
    if (sink->level() <= ::Er::Log2::Level::Debug) \
        ::Er::Log2::debug(sink, format, ##__VA_ARGS__)


#define ServerTraceIndent(format, ...) \
    if (::Er::Log2::get()->level() <= ::Er::Log2::Level::Debug) \
        ::Er::Log2::write(::Er::Log2::get(), ::Er::Log2::Level::Debug, format, ##__VA_ARGS__); \
    ::Er::Log2::IndentScope __ids(::Er::Log2::get(), ::Er::Log2::Level::Debug)

#define ServerTraceIndent2(sink, format, ...) \
    if (sink->level() <= ::Er::Log2::Level::Debug) \
        ::Er::Log2::write(sink, ::Er::Log2::Level::Debug, format, ##__VA_ARGS__); \
    ::Er::Log2::IndentScope __ids(sink, ::Er::Log2::Level::Debug)

#else // !ER_GRPC_SERVER_TRACE

#define ServerTrace(format, ...)                     ((void)0)
#define ServerTrace2(sink, format, ...)              ((void)0)

#define ServerTraceIndent(format, ...)               ((void)0)
#define ServerTraceIndent2(sink, format, ...)        ((void)0)

#endif // !ER_GRPC_SERVER_TRACE