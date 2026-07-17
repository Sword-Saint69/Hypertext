add_library(hypercore_memory STATIC
    Allocator.cpp
)

target_include_directories(hypercore_memory
    PUBLIC  ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(hypercore_memory
    PUBLIC  hypercore_warnings
    PRIVATE hypercore_release_flags
            spdlog::spdlog
)

set_target_properties(hypercore_memory PROPERTIES
    CXX_STANDARD          23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS        OFF
)
