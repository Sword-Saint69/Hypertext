add_executable(hypercore_tests
    test_smoke.cpp
    test_block.cpp
    test_allocator.cpp
)

target_link_libraries(hypercore_tests
    PRIVATE hypercore_core
            hypercore_memory
            hypercore_warnings
            GTest::gtest_main
            spdlog::spdlog
)

set_target_properties(hypercore_tests PROPERTIES
    CXX_STANDARD          23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS        OFF
)

# Register with CTest
include(GoogleTest)
gtest_discover_tests(hypercore_tests)
