add_executable(htx
    main.cpp
)

target_link_libraries(htx
    PRIVATE hypercore_core
            hypercore_memory
            hypercore_analysis
            hypercore_warnings
            hypercore_release_flags
            spdlog::spdlog
            CLI11::CLI11
)

set_target_properties(htx PROPERTIES
    CXX_STANDARD          23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS        OFF
    OUTPUT_NAME           htx
)

# Install the CLI binary
install(TARGETS htx
    RUNTIME DESTINATION bin
)
