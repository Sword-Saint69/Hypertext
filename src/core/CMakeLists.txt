add_library(hypercore_core STATIC
    Block.cpp
)

target_include_directories(hypercore_core
    PUBLIC  ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(hypercore_core
    PUBLIC  hypercore_warnings
    PRIVATE hypercore_release_flags
            spdlog::spdlog
)

set_target_properties(hypercore_core PROPERTIES
    CXX_STANDARD          23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS        OFF
)
