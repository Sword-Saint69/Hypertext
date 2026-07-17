add_library(hypercore_analysis STATIC
    TextAnalyzer.cpp
    FileAnalyzer.cpp
)

target_include_directories(hypercore_analysis
    PUBLIC  ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(hypercore_analysis
    PUBLIC  hypercore_core
            hypercore_warnings
    PRIVATE hypercore_release_flags
)

set_target_properties(hypercore_analysis PROPERTIES
    CXX_STANDARD          23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS        OFF
)
