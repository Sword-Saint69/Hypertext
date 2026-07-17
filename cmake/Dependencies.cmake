# ─────────────────────────────────────────────────────────────────────────────
# Dependencies.cmake
# Downloads and configures all third-party dependencies via FetchContent.
# ─────────────────────────────────────────────────────────────────────────────
include(FetchContent)

# Speed up repeated configuration by not re-fetching if already downloaded
set(FETCHCONTENT_QUIET OFF)

# ── GoogleTest ───────────────────────────────────────────────────────────────
if(HYPERCORE_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY  https://github.com/google/googletest.git
        GIT_TAG         v1.14.0
        GIT_SHALLOW     TRUE
    )
    # Prevent GoogleTest from overriding compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# ── spdlog (logging) ─────────────────────────────────────────────────────────
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY  https://github.com/gabime/spdlog.git
    GIT_TAG         v1.13.0
    GIT_SHALLOW     TRUE
)
set(SPDLOG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spdlog)

# ── CLI11 (command-line parsing) ─────────────────────────────────────────────
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY  https://github.com/CLIUtils/CLI11.git
    GIT_TAG         v2.4.1
    GIT_SHALLOW     TRUE
)
FetchContent_MakeAvailable(CLI11)

# ── BLAKE3 (cryptographic hashing) ───────────────────────────────────────────
FetchContent_Declare(
    blake3
    GIT_REPOSITORY  https://github.com/BLAKE3-team/BLAKE3.git
    GIT_TAG         1.5.0
    GIT_SHALLOW     TRUE
    SOURCE_SUBDIR   c
)
set(BLAKE3_NO_TESTING ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(blake3)

# ── Google Benchmark (micro-benchmarks) — optional ───────────────────────────
if(HYPERCORE_BUILD_TOOLS)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY  https://github.com/google/benchmark.git
        GIT_TAG         v1.8.4
        GIT_SHALLOW     TRUE
    )
    set(BENCHMARK_ENABLE_TESTING        OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS    OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)
endif()
