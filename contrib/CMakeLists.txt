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
