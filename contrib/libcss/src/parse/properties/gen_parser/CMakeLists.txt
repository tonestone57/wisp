
add_executable(gen_parser
    ../css_property_parser_gen.c
)

# Ensure gen_parser is not compiled with sanitizers and disabled optimizations/vectorization on Haiku.
# This prevents alignment-sensitive vectorized stack-initialization instructions that cause exit status 4 crashes.
# CMake does not define a built-in HAIKU variable, so detect the platform
# through CMAKE_SYSTEM_NAME instead.
if(CMAKE_SYSTEM_NAME STREQUAL "Haiku" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set_target_properties(gen_parser PROPERTIES
        COMPILE_FLAGS "-O0 -fno-tree-vectorize -fno-sanitize=address,undefined -fno-sanitize-recover=all"
        LINK_FLAGS "-fno-sanitize=address,undefined"
    )
    target_compile_options(gen_parser PRIVATE "-O0" "-fno-tree-vectorize" "-fno-sanitize=address,undefined")
    target_link_options(gen_parser PRIVATE "-fno-sanitize=address,undefined")
endif()

