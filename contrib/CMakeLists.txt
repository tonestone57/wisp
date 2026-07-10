add_subdirectory(libcss)
add_subdirectory(libdom)
add_subdirectory(libhubbub)
add_subdirectory(libnsbmp)
add_subdirectory(libnsgif)
add_subdirectory(libnsutils)
add_subdirectory(libparserutils)
add_subdirectory(libsvgtiny)

set(AVIF_CODEC_AOM SYSTEM CACHE STRING "")
set(AVIF_CODEC_DAV1D SYSTEM CACHE STRING "")
set(AVIF_BUILD_APPS OFF CACHE BOOL "")
set(AVIF_BUILD_EXAMPLES OFF CACHE BOOL "")
set(AVIF_BUILD_TESTS OFF CACHE BOOL "")
set(AVIF_LIBYUV OFF CACHE STRING "")
set(AVIF_ZLIBPNG OFF CACHE STRING "")
set(AVIF_JPEG OFF CACHE STRING "")
add_subdirectory(libavif)

# QuickJS-ng needs to be compiled with -fPIC for linking into shared library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(quickjs-ng)

if(WISP_USE_BLEND2D)
    # Configure JIT / asmjit based on target CPU capabilities (SSE2 or ARM64 required, RISC-V not supported)
    set(WISP_DISABLE_JIT OFF)
    if(WISP_TARGET_WINXP)
        set(WISP_DISABLE_JIT ON)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv" OR CMAKE_SYSTEM_PROCESSOR MATCHES "RISCV")
        set(WISP_DISABLE_JIT ON)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i.86" OR CMAKE_SYSTEM_PROCESSOR MATCHES "x86" OR CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
        if(CMAKE_C_FLAGS MATCHES "-mno-sse2" OR CMAKE_CXX_FLAGS MATCHES "-mno-sse2")
            set(WISP_DISABLE_JIT ON)
        endif()
    endif()

    if(WISP_DISABLE_JIT)
        set(BLEND2D_NO_JIT ON CACHE BOOL "Disable 'blend2d' JIT pipelines" FORCE)
        message(STATUS "Blend2D JIT (asmjit) is DISABLED (CPU does not support SSE2/ARM64, or is RISC-V)")
    else()
        set(BLEND2D_NO_JIT OFF CACHE BOOL "Enable 'blend2d' JIT pipelines" FORCE)
        message(STATUS "Blend2D JIT (asmjit) is ENABLED (CPU supports SSE2 or ARM64)")
    endif()

    set(BLEND2D_STATIC ON CACHE BOOL "")
    set(BLEND2D_NO_INSTALL ON CACHE BOOL "")

    # Ensure asmjit is available for JIT support in Blend2D
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/blend2d/3rdparty/asmjit/CMakeLists.txt")
        include(FetchContent)
        FetchContent_Declare(
            asmjit
            GIT_REPOSITORY https://github.com/asmjit/asmjit.git
            GIT_TAG        0bd5787b54b575ed94bf32ac452153b34385c514 # Matching a known stable commit
        )
        FetchContent_MakeAvailable(asmjit)
        set(ASMJIT_DIR "${asmjit_SOURCE_DIR}" CACHE PATH "Location of 'asmjit'")
    endif()

    add_subdirectory(blend2d EXCLUDE_FROM_ALL)
endif()
