if(ER_WINDOWS)
    set(PLATFORM_SOURCES
        logger/win32_debugger_sink.cxx
        system/win32_error.cxx
        util/utf16_win32.cxx
    )
    
    set(PLATFORM_HEADERS
        ../../include/erebus/system/system/win32_error.hxx
    )
    
elseif(ER_POSIX)
    set(PLATFORM_SOURCES
	system/signal_handler_posix.cxx        
    )
    
    set(PLATFORM_HEADERS
    	../../include/erebus/system/system/signal_handler_posix.hxx    
    )
    
endif()

add_library(erebus-system SHARED
    ../../include/erebus/system/assert.hxx
    ../../include/erebus/system/binary.hxx
    ../../include/erebus/system/bool.hxx
    ../../include/erebus/system/debug.hxx
    ../../include/erebus/system/empty.hxx
    ../../include/erebus/system/erebus.hxx
    ../../include/erebus/system/exception.hxx
    ../../include/erebus/system/flags.hxx
    ../../include/erebus/system/format.hxx
    ../../include/erebus/system/log.hxx
    ../../include/erebus/system/logger/null_logger.hxx
    ../../include/erebus/system/logger/simple_formatter.hxx
    ../../include/erebus/system/program.hxx
    ../../include/erebus/system/property.hxx
    ../../include/erebus/system/property_bag.hxx
    ../../include/erebus/system/property_info.hxx
    ../../include/erebus/system/result.hxx
    ../../include/erebus/system/system/packed_time.hxx
    ../../include/erebus/system/system/posix_error.hxx
    ../../include/erebus/system/system/process.hxx
    ../../include/erebus/system/system/thread.hxx
    ../../include/erebus/system/util/autoptr.hxx
    ../../include/erebus/system/util/errno.hxx
    ../../include/erebus/system/util/exception_util.hxx
    ../../include/erebus/system/util/generic_handle.hxx
    ../../include/erebus/system/util/null_mutex.hxx
    ../../include/erebus/system/util/utf16.hxx
    ../../include/erebus/system/waitable.hxx
    ${PLATFORM_HEADERS}
)

target_sources(erebus-system
	PRIVATE
        assert.cxx
        debug.cxx
        exception.cxx
	init.cxx
        logger/async_logger.cxx
        logger/file_sink.cxx
        logger/ostream_sink.cxx
        logger/simple_formatter.cxx
        program.cxx
        property.cxx
        property_info.cxx
        result.cxx
        system/packed_time.cxx
        system/posix_error.cxx
        system/process.cxx
        system/thread.cxx
        util/exception_util.cxx
        ${PLATFORM_SOURCES}
)

target_link_libraries(erebus-system
    PUBLIC
    fmt::fmt
    boost::boost
)

target_compile_definitions(erebus-system PRIVATE ER_SYSTEM_EXPORTS)
