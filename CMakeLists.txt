cmake_minimum_required(VERSION 3.25)

# ─────────────────────────────────────────────────────────────────────────────
# Project declaration
# ─────────────────────────────────────────────────────────────────────────────
project(hypercore
    VERSION     4.3.0
    DESCRIPTION "Next-generation hybrid CPU-GPU lossless text compression"
    LANGUAGES   CXX
)

# Optional CUDA support — detected automatically
include(CheckLanguage)
check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    set(HYPERCORE_CUDA_ENABLED TRUE)
    message(STATUS "HyperCore: CUDA enabled (${CMAKE_CUDA_COMPILER})")
else()
    set(HYPERCORE_CUDA_ENABLED FALSE)
    message(STATUS "HyperCore: CUDA not found — building CPU-only")
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Build options
# ─────────────────────────────────────────────────────────────────────────────
option(HYPERCORE_BUILD_TESTS    "Build unit tests"        ON)
option(HYPERCORE_BUILD_TOOLS    "Build benchmark tools"   ON)
option(HYPERCORE_ENABLE_OPENMP  "Enable OpenMP support"   ON)
option(HYPERCORE_ENABLE_TBB     "Enable Intel TBB support" ON)

# ─────────────────────────────────────────────────────────────────────────────
# CMake modules
# ─────────────────────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerFlags)
include(Dependencies)

# ─────────────────────────────────────────────────────────────────────────────
# Optional: OpenMP
# ─────────────────────────────────────────────────────────────────────────────
if(HYPERCORE_ENABLE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        message(STATUS "HyperCore: OpenMP enabled")
    else()
        message(STATUS "HyperCore: OpenMP not found")
    endif()
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Optional: Intel TBB
# ─────────────────────────────────────────────────────────────────────────────
if(HYPERCORE_ENABLE_TBB)
    find_package(TBB QUIET)
    if(TBB_FOUND)
        message(STATUS "HyperCore: Intel TBB enabled")
    else()
        message(STATUS "HyperCore: Intel TBB not found — using std::execution fallback")
    endif()
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Global include directory
# ─────────────────────────────────────────────────────────────────────────────
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ─────────────────────────────────────────────────────────────────────────────
# Subdirectories
# ─────────────────────────────────────────────────────────────────────────────
add_subdirectory(src)

if(HYPERCORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Summary
# ─────────────────────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "══════════════════════════════════════════")
message(STATUS "  HyperCore v${PROJECT_VERSION} — Configuration")
message(STATUS "══════════════════════════════════════════")
message(STATUS "  Build type    : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard  : C++23")
message(STATUS "  CUDA          : ${HYPERCORE_CUDA_ENABLED}")
message(STATUS "  OpenMP        : ${OpenMP_CXX_FOUND}")
message(STATUS "  Intel TBB     : ${TBB_FOUND}")
message(STATUS "  Tests         : ${HYPERCORE_BUILD_TESTS}")
message(STATUS "══════════════════════════════════════════")
message(STATUS "")
