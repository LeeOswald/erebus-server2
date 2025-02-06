
enable_testing()

add_executable(
    erebus-system-tests
    common.hpp
    flags.cpp
    luaxx_class.cpp
    luaxx_error.cpp
    luaxx_exception.cpp
    luaxx_int64.cpp
    luaxx_interop.cpp
    luaxx_metatable.cpp
    luaxx_obj.cpp
    luaxx_property.cpp
    luaxx_reference.cpp
    luaxx_selector.cpp
    main.cpp
    property.cpp
    property_bag.cpp
)

target_link_libraries(erebus-system-tests PRIVATE gtest::gtest erebus-system)


