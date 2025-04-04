if(ER_WINDOWS)
    set(PLATFORM_SOURCES
        logger/win32_debugger_sink2.cxx
        system/win32_error.cxx
        util/utf16_win32.cxx
    )
    
    set(PLATFORM_HEADERS
    	../../include/erebus/system/logger/win32_debugger_sink2.hxx
        ../../include/erebus/system/system/win32_error.hxx
    )
    
elseif(ER_POSIX)
    set(PLATFORM_SOURCES
    	logger/syslog_linux_sink2.cxx
    )
    
    set(PLATFORM_HEADERS
     	../../include/erebus/system/logger/syslog_linux_sink2.hxx
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
    ../../include/erebus/system/logger2.hxx
    ../../include/erebus/system/logger/file_sink2.hxx
    ../../include/erebus/system/logger/null_logger2.hxx
    ../../include/erebus/system/logger/ostream_sink2.hxx
    ../../include/erebus/system/logger/simple_formatter2.hxx
    ../../include/erebus/system/luaxx.hxx
    ../../include/erebus/system/luaxx/luaxx_base_fun.hxx
    ../../include/erebus/system/luaxx/luaxx_class.hxx
    ../../include/erebus/system/luaxx/luaxx_class_fun.hxx
    ../../include/erebus/system/luaxx/luaxx_ctor.hxx
    ../../include/erebus/system/luaxx/luaxx_dtor.hxx
    ../../include/erebus/system/luaxx/luaxx_exception_handler.hxx
    ../../include/erebus/system/luaxx/luaxx_exception_types.hxx
    ../../include/erebus/system/luaxx/luaxx_fun.hxx
    ../../include/erebus/system/luaxx/luaxx_function.hxx
    ../../include/erebus/system/luaxx/luaxx_int64.hxx
    ../../include/erebus/system/luaxx/luaxx_lua_name.hxx
    ../../include/erebus/system/luaxx/luaxx_lua_ref.hxx
    ../../include/erebus/system/luaxx/luaxx_metatable_registry.hxx
    ../../include/erebus/system/luaxx/luaxx_obj.hxx
    ../../include/erebus/system/luaxx/luaxx_obj_fun.hxx
    ../../include/erebus/system/luaxx/luaxx_primitives.hxx
    ../../include/erebus/system/luaxx/luaxx_property.hxx
    ../../include/erebus/system/luaxx/luaxx_references.hxx
    ../../include/erebus/system/luaxx/luaxx_registry.hxx
    ../../include/erebus/system/luaxx/luaxx_resource_handler.hxx
    ../../include/erebus/system/luaxx/luaxx_selector.hxx
    ../../include/erebus/system/luaxx/luaxx_state.hxx
    ../../include/erebus/system/luaxx/luaxx_traits.hxx
    ../../include/erebus/system/luaxx/luaxx_tuple.hxx
    ../../include/erebus/system/luaxx/luaxx_util.hxx
    ../../include/erebus/system/program.hxx
    ../../include/erebus/system/property.hxx
    ../../include/erebus/system/property_bag.hxx
    ../../include/erebus/system/property_info.hxx
    ../../include/erebus/system/result.hxx
    ../../include/erebus/system/system/packed_time.hxx
    ../../include/erebus/system/system/posix_error.hxx
    ../../include/erebus/system/system/process.hxx
    ../../include/erebus/system/system/thread.hxx
    ../../include/erebus/system/type_id.hxx
    ../../include/erebus/system/util/autoptr.hxx
    ../../include/erebus/system/util/errno.hxx
    ../../include/erebus/system/util/exception_util.hxx
    ../../include/erebus/system/util/generic_handle.hxx
    ../../include/erebus/system/util/null_mutex.hxx
    ../../include/erebus/system/util/thread_data.hxx
    ../../include/erebus/system/util/utf16.hxx
    ../../include/erebus/system/waitable.hxx
    ${PLATFORM_HEADERS}
)

target_sources(erebus-system
	PRIVATE
        assert.cxx
        debug.cxx
        exception.cxx
        logger/logger2.cxx
        logger/async_logger2.cxx
        logger/file_sink2.cxx
        logger/ostream_sink2.cxx
        logger/simple_formatter2.cxx
        logger/sync_logger.cxx
        logger/tee2.cxx
        luaxx.cxx
        luaxx/luaxx_base_fun.cxx
        luaxx/luaxx_exception_handler.cxx
        luaxx/luaxx_int64.cxx
        luaxx/luaxx_metatable_registry.cxx
        luaxx/luaxx_property.cxx
        luaxx/luaxx_selector.cxx
        luaxx/luaxx_state.cxx
        luaxx/luaxx_util.cxx
        program.cxx
        property.cxx
        property_info.cxx
        result.cxx
        system/packed_time.cxx
        system/posix_error.cxx
        system/process.cxx
        system/thread.cxx
        type_id.cxx
        util/exception_util.cxx
        ${PLATFORM_SOURCES}
)

target_link_libraries(
    erebus-system
    PUBLIC
    fmt::fmt
    ${Boost_LIBRARIES}
    ${lua_LIBRARIES}
)

target_compile_definitions(erebus-system PRIVATE ER_SYSTEM_EXPORTS)

set_property(TARGET erebus-system PROPERTY PREFIX "")

if(NOT ER_BUILD_CLIENT_LIBS_ONLY)
    add_subdirectory(tests)
endif()
