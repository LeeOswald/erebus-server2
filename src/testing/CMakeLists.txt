add_library(erebus-testing STATIC
    ../../include/erebus/testing/test_application.hxx
    ../../include/erebus/testing/test_event_listener.hxx
    test_application.cxx
    test_event_listener.cxx
)

target_link_libraries(erebus-testing PUBLIC gtest::gtest erebus::system)