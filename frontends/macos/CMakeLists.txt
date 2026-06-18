add_library(macos_frontend STATIC audio.c)
target_include_directories(macos_frontend PRIVATE ${CMAKE_SOURCE_DIR}/include)
