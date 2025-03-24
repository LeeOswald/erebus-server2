
add_executable(
    erebus-grpc-tests
    common.hpp
    main.cpp
)

target_link_libraries(erebus-grpc-tests PRIVATE erebus::testing erebus::system erebus::grpc)

add_test(NAME erebus-grpc COMMAND erebus-grpc-tests)


