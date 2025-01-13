
enable_testing()

add_executable(
    erebus-system-tests
    common.hpp
    flags.cpp
    main.cpp
    property.cpp
)

target_link_libraries(erebus-system-tests PRIVATE gtest::gtest erebus-system)


