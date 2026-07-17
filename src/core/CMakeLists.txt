add_library(hypercore_core STATIC
    Block.cpp
    ContextMixer.cpp
    CharacterPredictor.cpp
    IPredictor.cpp
    BitWriter.cpp
    RansEncoder.cpp
    ArchiveWriter.cpp
)

target_include_directories(hypercore_core
    PUBLIC  ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(hypercore_core
    PUBLIC  hypercore_warnings
            blake3
    PRIVATE hypercore_release_flags
            spdlog::spdlog
)

set_target_properties(hypercore_core PROPERTIES
    CXX_STANDARD          23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS        OFF
)

include(CheckCXXCompilerFlag)
if(MSVC)
    check_cxx_compiler_flag("/arch:AVX2" HAS_AVX2)
    if(HAS_AVX2)
        target_compile_options(hypercore_core PRIVATE "/arch:AVX2")
        target_compile_definitions(hypercore_core PUBLIC HYPERCORE_HAS_AVX2)
    endif()
else()
    check_cxx_compiler_flag("-mavx2" HAS_AVX2)
    if(HAS_AVX2)
        target_compile_options(hypercore_core PRIVATE "-mavx2")
        target_compile_definitions(hypercore_core PUBLIC HYPERCORE_HAS_AVX2)
    endif()
endif()
