# ─────────────────────────────────────────────────────────────────────────────
# CompilerFlags.cmake
# Sets C++23, CUDA standards, and warning flags for all targets.
# ─────────────────────────────────────────────────────────────────────────────

# ── C++ Standard ────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)   # No GNU extensions — strict standard

# ── CUDA Standard ───────────────────────────────────────────────────────────
if(HYPERCORE_CUDA_ENABLED)
    set(CMAKE_CUDA_STANDARD          20)   # CUDA does not yet support C++23
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_EXTENSIONS        OFF)

    # Default GPU architecture: SM 7.0 (Volta/Turing) through SM 9.0 (Hopper)
    # Override with: cmake -DCMAKE_CUDA_ARCHITECTURES="80;86;89;90"
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES "70;80;86;89;90")
    endif()
    message(STATUS "HyperCore: CUDA architectures = ${CMAKE_CUDA_ARCHITECTURES}")
endif()

# ── Warning Flags ────────────────────────────────────────────────────────────
# Applied to the hypercore_warnings INTERFACE target (linked by all targets).
add_library(hypercore_warnings INTERFACE)

if(MSVC)
    target_compile_options(hypercore_warnings INTERFACE
        /W4         # Warning level 4
        /WX         # Treat warnings as errors
        /wd4068     # Unknown pragma (suppress CUDA pragmas in MSVC)
        /permissive-# Strict standard conformance
        /Zc:__cplusplus
    )
else()
    target_compile_options(hypercore_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wmisleading-indentation
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Werror          # Treat warnings as errors
    )
endif()

# ── Optimization Flags (Release) ─────────────────────────────────────────────
add_library(hypercore_release_flags INTERFACE)
if(NOT MSVC)
    target_compile_options(hypercore_release_flags INTERFACE
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-march=native>
        $<$<CONFIG:Release>:-DNDEBUG>
        $<$<CONFIG:Release>:-fomit-frame-pointer>
    )
else()
    target_compile_options(hypercore_release_flags INTERFACE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/DNDEBUG>
    )
endif()
