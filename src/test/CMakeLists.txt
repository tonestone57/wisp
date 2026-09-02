# NeoSurf Tests

# Guard: Only enable tests if NEOSURF_ENABLE_TESTS is ON
# This is automatically ON for Debug builds or can be set via cmake option
if(NOT WISP_ENABLE_TESTS)
  return()
endif()

# Check for 'check' unit test framework
pkg_check_modules(CHECK check)
if(NOT CHECK_FOUND)
  message(FATAL_ERROR "Missing 'check' unit test framework; install it or set WISP_ENABLE_TESTS=OFF")
endif()

include(CTest)

# Common includes for all tests
set(TEST_COMMON_INCLUDES
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/include/wisp
  ${CMAKE_SOURCE_DIR}/frontends
  ${CMAKE_SOURCE_DIR}/src/content/handlers
  ${CHECK_INCLUDE_DIRS}
)

# Function to add a NeoSurf test executable
function(add_wisp_test name)
  set(TEST_SRCS ${ARGN})
  if (";${TEST_SRCS};" MATCHES "idna\\.c" OR ";${TEST_SRCS};" MATCHES "utf8\\.c")
    list(APPEND TEST_SRCS "${CMAKE_SOURCE_DIR}/src/utils/utf8proc_wrapper.c")
  endif()
  add_executable(${name} ${TEST_SRCS})
  target_include_directories(${name} PRIVATE ${TEST_COMMON_INCLUDES})
  target_compile_definitions(${name} PRIVATE
    nsgtk
    WISP_BUILTIN_LOG_FILTER=\"level:WARNING\"
    WISP_BUILTIN_VERBOSE_FILTER=\"level:DEBUG\"
    WITH_UTF8PROC
    WISP_TEST_DATA_DIR=\"${CMAKE_SOURCE_DIR}/src/test/data\"
  HTML5TEST_DATA_DIR=\"${CMAKE_SOURCE_DIR}/src/test/data/html5test\"
  )
  target_link_libraries(${name} PUBLIC ${WISP_COMMON_LIBS} ${CHECK_LIBRARIES})
  add_test(NAME ${name} COMMAND ${name})
  set_tests_properties(${name} PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  set(TEST_ENV_PATH "C:/msys64/mingw64/bin\;C:/msys64/usr/bin\;$ENV{PATH}")
  set_tests_properties(${name} PROPERTIES ENVIRONMENT "PATH=${TEST_ENV_PATH};CK_VERBOSITY=verbose;CK_FORK=yes")

  if(WIN32)
    set(SVGTINY_RUNTIME)
    if(TARGET svgtiny)
      get_target_property(SVGTINY_TYPE svgtiny TYPE)
      if(SVGTINY_TYPE STREQUAL "SHARED_LIBRARY" OR SVGTINY_TYPE STREQUAL "MODULE_LIBRARY")
        set(SVGTINY_RUNTIME "$<TARGET_FILE:svgtiny>")
      endif()
    endif()
    if(SVGTINY_RUNTIME)
      add_custom_command(TARGET ${name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SVGTINY_RUNTIME}" "$<TARGET_FILE_DIR:${name}>/"
      )
    endif()
    add_custom_command(TARGET ${name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/contrib/libdom/libdom.dll" "$<TARGET_FILE_DIR:${name}>/"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/contrib/libnsutils/libnsutils.dll" "$<TARGET_FILE_DIR:${name}>/"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/contrib/libparserutils/libparserutils.dll" "$<TARGET_FILE_DIR:${name}>/"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/contrib/libnsbmp/libnsbmp.dll" "$<TARGET_FILE_DIR:${name}>/"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/contrib/libnsgif/libnsgif.dll" "$<TARGET_FILE_DIR:${name}>/"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/contrib/libcss/libcss.dll" "$<TARGET_FILE_DIR:${name}>/"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/contrib/libhubbub/libhubbub.dll" "$<TARGET_FILE_DIR:${name}>/"
    )
  endif()
endfunction()

# ============================================================================
# URL Tests
# ============================================================================

set(NSURL_SOURCES
  ${CMAKE_SOURCE_DIR}/src/utils/nsurl/nsurl.c
  ${CMAKE_SOURCE_DIR}/src/utils/nsurl/parse.c
  ${CMAKE_SOURCE_DIR}/src/utils/idna.c
  ${CMAKE_SOURCE_DIR}/src/utils/punycode.c
  ${CMAKE_SOURCE_DIR}/src/utils/corestrings.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/nsurl.c
)
add_wisp_test(nsurl ${NSURL_SOURCES})

set(TEST_NSURL_SOURCES
  ${CMAKE_SOURCE_DIR}/src/utils/nsurl/nsurl.c
  ${CMAKE_SOURCE_DIR}/src/utils/nsurl/parse.c
  ${CMAKE_SOURCE_DIR}/src/utils/idna.c
  ${CMAKE_SOURCE_DIR}/src/utils/punycode.c
  ${CMAKE_SOURCE_DIR}/src/utils/corestrings.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/test_nsurl.c
)
add_wisp_test(test_nsurl ${TEST_NSURL_SOURCES})

set(NSURL_PARSE_SOURCES
  ${CMAKE_SOURCE_DIR}/src/utils/nsurl/nsurl.c
  ${CMAKE_SOURCE_DIR}/src/utils/nsurl/parse.c
  ${CMAKE_SOURCE_DIR}/src/utils/idna.c
  ${CMAKE_SOURCE_DIR}/src/utils/punycode.c
  ${CMAKE_SOURCE_DIR}/src/utils/corestrings.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/test_nsurl_parse.c
)
add_wisp_test(test_nsurl_parse ${NSURL_PARSE_SOURCES})


set(URLDBTEST_SOURCES
  ${CMAKE_SOURCE_DIR}/src/utils/nsurl/nsurl.c
  ${CMAKE_SOURCE_DIR}/src/utils/nsurl/parse.c
  ${CMAKE_SOURCE_DIR}/src/utils/idna.c
  ${CMAKE_SOURCE_DIR}/src/utils/punycode.c
  ${CMAKE_SOURCE_DIR}/src/utils/bloom.c
  ${CMAKE_SOURCE_DIR}/src/utils/nsoption.c
  ${CMAKE_SOURCE_DIR}/src/utils/corestrings.c
  ${CMAKE_SOURCE_DIR}/src/utils/time.c
  ${CMAKE_SOURCE_DIR}/src/utils/hashtable.c
  ${CMAKE_SOURCE_DIR}/src/utils/messages.c
  ${CMAKE_SOURCE_DIR}/src/utils/utils.c
  ${CMAKE_SOURCE_DIR}/src/utils/http/primitives.c
  ${CMAKE_SOURCE_DIR}/src/utils/http/generics.c
  ${CMAKE_SOURCE_DIR}/src/utils/http/strict-transport-security.c
  ${CMAKE_SOURCE_DIR}/src/content/urldb.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/urldbtest.c
)
add_wisp_test(urldbtest ${URLDBTEST_SOURCES})

add_wisp_test(test_url
  ${CMAKE_SOURCE_DIR}/src/utils/url.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/test_url.c
)

add_wisp_test(test_idna
  ${CMAKE_SOURCE_DIR}/src/test/test_idna.c
)
target_link_libraries(test_idna PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_punycode
  ${CMAKE_SOURCE_DIR}/src/test/test_punycode.c
)
target_link_libraries(test_punycode PRIVATE wisp ${CHECK_LIBRARIES})





add_wisp_test(test_filepath
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/utils/filepath.c
  ${CMAKE_SOURCE_DIR}/src/test/test_filepath.c
)

add_wisp_test(csp_test
  ${CMAKE_SOURCE_DIR}/src/content/csp.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/test/test_csp.c
  ${CMAKE_SOURCE_DIR}/src/utils/nsurl/nsurl.c
  ${CMAKE_SOURCE_DIR}/src/utils/nsurl/parse.c
  ${CMAKE_SOURCE_DIR}/src/utils/idna.c
  ${CMAKE_SOURCE_DIR}/src/utils/punycode.c
  ${CMAKE_SOURCE_DIR}/src/utils/utils.c
  ${CMAKE_SOURCE_DIR}/src/utils/messages.c
  ${CMAKE_SOURCE_DIR}/src/utils/hashtable.c
  ${CMAKE_SOURCE_DIR}/src/utils/corestrings.c
)

# ============================================================================
# Utility Tests
# ============================================================================

add_wisp_test(messages
  ${CMAKE_SOURCE_DIR}/src/utils/messages.c
  ${CMAKE_SOURCE_DIR}/src/utils/hashtable.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/messages.c
)

add_wisp_test(test_nsoption
  ${CMAKE_SOURCE_DIR}/src/utils/nsoption.c
  ${CMAKE_SOURCE_DIR}/src/utils/utils.c
  ${CMAKE_SOURCE_DIR}/src/utils/messages.c
  ${CMAKE_SOURCE_DIR}/src/utils/hashtable.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/test_nsoption.c
)

add_wisp_test(http3_test
  ${CMAKE_SOURCE_DIR}/src/utils/nsoption.c
  ${CMAKE_SOURCE_DIR}/src/utils/utils.c
  ${CMAKE_SOURCE_DIR}/src/utils/messages.c
  ${CMAKE_SOURCE_DIR}/src/utils/hashtable.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/http3_test.c
)

add_wisp_test(test_bloom
  ${CMAKE_SOURCE_DIR}/src/utils/bloom.c
  ${CMAKE_SOURCE_DIR}/src/test/test_bloom.c
)

add_wisp_test(test_hashtable
  ${CMAKE_SOURCE_DIR}/src/utils/hashtable.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/test_hashtable.c
)

add_wisp_test(test_core_buffer
  ${CMAKE_SOURCE_DIR}/src/utils/core_buffer.c
  ${CMAKE_SOURCE_DIR}/src/test/test_core_buffer.c
)
target_link_libraries(test_core_buffer PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_http_primitives
  ${CMAKE_SOURCE_DIR}/src/test/test_http_primitives.c
)
target_link_libraries(test_http_primitives PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(utils
  ${CMAKE_SOURCE_DIR}/src/utils/nsurl/nsurl.c
  ${CMAKE_SOURCE_DIR}/src/utils/nsurl/parse.c
  ${CMAKE_SOURCE_DIR}/src/utils/idna.c
  ${CMAKE_SOURCE_DIR}/src/utils/punycode.c
  ${CMAKE_SOURCE_DIR}/src/utils/utils.c
  ${CMAKE_SOURCE_DIR}/src/utils/messages.c
  ${CMAKE_SOURCE_DIR}/src/utils/hashtable.c
  ${CMAKE_SOURCE_DIR}/src/utils/corestrings.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/utils.c
)

add_wisp_test(test_ipc_partial_read
  ${CMAKE_SOURCE_DIR}/src/test/test_ipc_partial_read.c
)
target_link_libraries(test_ipc_partial_read PRIVATE wisp)

add_wisp_test(test_time
  ${CMAKE_SOURCE_DIR}/src/utils/time.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/test_time.c
)

add_wisp_test(useragent
  ${CMAKE_SOURCE_DIR}/src/utils/useragent.c
  ${CMAKE_SOURCE_DIR}/src/desktop/version.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/malloc_fig.c
  ${CMAKE_SOURCE_DIR}/src/test/useragent.c
)

add_wisp_test(mimesniff
  ${CMAKE_SOURCE_DIR}/src/utils/nsurl/nsurl.c
  ${CMAKE_SOURCE_DIR}/src/utils/nsurl/parse.c
  ${CMAKE_SOURCE_DIR}/src/utils/idna.c
  ${CMAKE_SOURCE_DIR}/src/utils/punycode.c
  ${CMAKE_SOURCE_DIR}/src/utils/hashtable.c
  ${CMAKE_SOURCE_DIR}/src/utils/corestrings.c
  ${CMAKE_SOURCE_DIR}/src/utils/http/generics.c
  ${CMAKE_SOURCE_DIR}/src/utils/http/content-type.c
  ${CMAKE_SOURCE_DIR}/src/utils/http/primitives.c
  ${CMAKE_SOURCE_DIR}/src/utils/messages.c
  ${CMAKE_SOURCE_DIR}/src/utils/http/parameter.c
  ${CMAKE_SOURCE_DIR}/src/content/mimesniff.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/mimesniff.c
)

# ============================================================================
# Renderer Tests
# ============================================================================

add_wisp_test(html_redraw_borders_test
  ${CMAKE_SOURCE_DIR}/src/content/handlers/html/redraw_border.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/html_redraw_borders_test.c
)

add_wisp_test(layout_flex_test
  ${CMAKE_SOURCE_DIR}/src/content/handlers/html/layout_flex.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/layout_flex_stubs.c
  ${CMAKE_SOURCE_DIR}/src/test/layout_flex_test.c
)
# Define TESTING so layout_flex.c can use test stubs
target_compile_definitions(layout_flex_test PRIVATE TESTING)


add_wisp_test(svg_redraw_test
  ${CMAKE_SOURCE_DIR}/src/desktop/plot_style.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/svg_redraw_driver.c
  ${CMAKE_SOURCE_DIR}/src/test/svg_redraw_test.c
)

add_wisp_test(svg_redraw_extlink_test
  ${CMAKE_SOURCE_DIR}/src/desktop/plot_style.c
  ${CMAKE_SOURCE_DIR}/src/content/handlers/image/svg.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/content_stubs.c
  ${CMAKE_SOURCE_DIR}/src/test/svg_redraw_extlink_test.c
)

add_wisp_test(svg_redraw_comboflush_test
  ${CMAKE_SOURCE_DIR}/src/desktop/plot_style.c
  ${CMAKE_SOURCE_DIR}/src/content/handlers/image/svg.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/content_stubs.c
  ${CMAKE_SOURCE_DIR}/src/test/svg_redraw_comboflush_test.c
)

add_wisp_test(svg_dash_rects_test
  ${CMAKE_SOURCE_DIR}/src/desktop/plot_style.c
  ${CMAKE_SOURCE_DIR}/src/content/handlers/image/svg.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/content_stubs.c
  ${CMAKE_SOURCE_DIR}/src/test/svg_dash_rects_test.c
)

add_wisp_test(stacking_test
  ${CMAKE_SOURCE_DIR}/src/content/handlers/html/stacking.c
  ${CMAKE_SOURCE_DIR}/src/utils/utils.c
  ${CMAKE_SOURCE_DIR}/src/utils/messages.c
  ${CMAKE_SOURCE_DIR}/src/utils/hashtable.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/stacking_test.c
)

add_wisp_test(box_construct_race_test
  ${CMAKE_SOURCE_DIR}/src/utils/talloc.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/box_construct_race_test.c
)

add_wisp_test(stylesheet_cascade_gate_test
  ${CMAKE_SOURCE_DIR}/src/utils/talloc.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/stylesheet_cascade_gate_test.c
)

add_wisp_test(grid_layout_test
  ${CMAKE_SOURCE_DIR}/src/content/handlers/html/layout_grid.c
  ${CMAKE_SOURCE_DIR}/src/utils/libdom.c
  ${CMAKE_SOURCE_DIR}/src/utils/talloc.c
  ${CMAKE_SOURCE_DIR}/src/utils/corestrings.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/layout_flex_stubs.c
  ${CMAKE_SOURCE_DIR}/src/test/grid_layout_test.c
)
# Define TESTING so layout_grid.c uses test stubs instead of calling layout_minmax_box
target_compile_definitions(grid_layout_test PRIVATE TESTING)

add_wisp_test(layout_inline_grid_test
  ${CMAKE_SOURCE_DIR}/src/content/handlers/html/layout_grid.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/layout_flex_stubs.c
  ${CMAKE_SOURCE_DIR}/src/test/layout_inline_grid_test.c
)
target_compile_definitions(layout_inline_grid_test PRIVATE TESTING)

add_wisp_test(grid_construct_test
  ${CMAKE_SOURCE_DIR}/src/utils/libdom.c
  ${CMAKE_SOURCE_DIR}/src/utils/talloc.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/content/handlers/html/table.c
  ${CMAKE_SOURCE_DIR}/src/test/grid_construct_test.c
)
add_wisp_test(transform_clip_test
  ${CMAKE_SOURCE_DIR}/src/content/handlers/html/redraw_helpers.c
  ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
  ${CMAKE_SOURCE_DIR}/src/test/transform_clip_test.c
)

# ============================================================================
# JavaScript Tests
# ============================================================================

add_wisp_test(test_quickjs
  ${CMAKE_SOURCE_DIR}/src/test/test_quickjs.c
)
add_dependencies(test_quickjs quickjs_bindings)
target_include_directories(test_quickjs PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/contrib/quickjs-ng
  ${CMAKE_SOURCE_DIR}/contrib/libnsutils/include
  ${CMAKE_BINARY_DIR}/quickjs
  ${CMAKE_SOURCE_DIR}/src/content/handlers/javascript/quickjs
)
target_link_libraries(test_quickjs PRIVATE qjs nsutils wisp)

add_wisp_test(test_html5test_runner
  ${CMAKE_SOURCE_DIR}/src/test/test_html5test_runner.c
)
add_dependencies(test_html5test_runner quickjs_bindings)
target_include_directories(test_html5test_runner PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/contrib/quickjs-ng
  ${CMAKE_SOURCE_DIR}/contrib/libnsutils/include
  ${CMAKE_BINARY_DIR}/quickjs
  ${CMAKE_SOURCE_DIR}/src/content/handlers/javascript/quickjs
)
target_link_libraries(test_html5test_runner PRIVATE qjs nsutils wisp)

add_wisp_test(test_html5test_full
  ${CMAKE_SOURCE_DIR}/src/test/test_html5test_full.c
)
add_dependencies(test_html5test_full quickjs_bindings)
target_include_directories(test_html5test_full PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/contrib/quickjs-ng
  ${CMAKE_SOURCE_DIR}/contrib/libnsutils/include
  ${CMAKE_BINARY_DIR}/quickjs
  ${CMAKE_SOURCE_DIR}/src/content/handlers/javascript/quickjs
)
target_link_libraries(test_html5test_full PRIVATE qjs nsutils wisp)

add_wisp_test(test_wisp_subsystem_pools
  ${CMAKE_SOURCE_DIR}/src/test/test_wisp_subsystem_pools.c
)
target_include_directories(test_wisp_subsystem_pools PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/quickjs-ng
  ${CMAKE_BINARY_DIR}/quickjs
  ${CMAKE_SOURCE_DIR}/src/content/handlers/javascript/quickjs
)
target_link_libraries(test_wisp_subsystem_pools PRIVATE qjs wisp)

add_wisp_test(test_web_workers
  ${CMAKE_SOURCE_DIR}/src/test/test_web_workers.c
)
target_include_directories(test_web_workers PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/quickjs-ng
  ${CMAKE_BINARY_DIR}/quickjs
  ${CMAKE_SOURCE_DIR}/src/content/handlers/javascript/quickjs
)
target_link_libraries(test_web_workers PRIVATE qjs wisp)

# ============================================================================
# Font-face Tests (simple test linked to neosurf lib)
# ============================================================================

add_executable(font_face_test font_face_test.c)
target_include_directories(font_face_test PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(font_face_test wisp)
add_test(NAME font_face_test COMMAND font_face_test)

# Margin collapse test — two-unit build to avoid enum collisions
# between Wisp headers (box.h) and libcss internal headers (opcodes.h)

# Style factory: only sees libcss internal headers
add_library(margin_collapse_style OBJECT layout_margin_collapse_style.c)
add_dependencies(margin_collapse_style css)
target_include_directories(margin_collapse_style PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/libcss/include
  ${CMAKE_SOURCE_DIR}/contrib/libcss/src
  ${CMAKE_BINARY_DIR}/contrib/libcss/src
  ${CMAKE_BINARY_DIR}/contrib/libcss/src/select
  ${CMAKE_BINARY_DIR}/contrib/libcss/src/generated
)

# Test executable: only sees Wisp headers
add_executable(layout_margin_collapse_test
  layout_margin_collapse_test.c
  $<TARGET_OBJECTS:margin_collapse_style>
)
target_include_directories(layout_margin_collapse_test PRIVATE
  ${TEST_COMMON_INCLUDES}
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(layout_margin_collapse_test wisp ${CHECK_LIBRARIES})
add_test(NAME layout_margin_collapse_test COMMAND layout_margin_collapse_test)
set_tests_properties(layout_margin_collapse_test PROPERTIES
  ENVIRONMENT "CK_VERBOSITY=verbose;CK_FORK=no"
)

# ============================================================================
# Unix-only Tests (require malloc_fig)
# ============================================================================

if(NOT WIN32)
  add_library(malloc_fig SHARED ${CMAKE_SOURCE_DIR}/src/test/malloc_fig.c)
  target_include_directories(malloc_fig PRIVATE ${CMAKE_SOURCE_DIR}/src)
  target_link_libraries(malloc_fig PUBLIC dl)

  add_wisp_test(hashmap
    ${CMAKE_SOURCE_DIR}/src/utils/nsurl/nsurl.c
    ${CMAKE_SOURCE_DIR}/src/utils/nsurl/parse.c
    ${CMAKE_SOURCE_DIR}/src/utils/idna.c
    ${CMAKE_SOURCE_DIR}/src/utils/punycode.c
    ${CMAKE_SOURCE_DIR}/src/utils/hashmap.c
    ${CMAKE_SOURCE_DIR}/src/utils/corestrings.c
    ${CMAKE_SOURCE_DIR}/src/test/log.c
    ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
    ${CMAKE_SOURCE_DIR}/src/test/hashmap.c
  )
  target_link_libraries(hashmap PRIVATE malloc_fig)

  add_wisp_test(corestrings
    ${CMAKE_SOURCE_DIR}/src/utils/nsurl/nsurl.c
    ${CMAKE_SOURCE_DIR}/src/utils/nsurl/parse.c
    ${CMAKE_SOURCE_DIR}/src/utils/idna.c
    ${CMAKE_SOURCE_DIR}/src/utils/punycode.c
    ${CMAKE_SOURCE_DIR}/src/utils/corestrings.c
    ${CMAKE_SOURCE_DIR}/src/test/log.c
    ${CMAKE_SOURCE_DIR}/src/test/guit_stub.c
    ${CMAKE_SOURCE_DIR}/src/test/corestrings.c
  )
  target_link_libraries(corestrings PRIVATE malloc_fig)

add_wisp_test(journal_test
  ${CMAKE_SOURCE_DIR}/src/test/journal_test.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
)
target_link_libraries(journal_test PRIVATE wisp)

add_wisp_test(test_utf8proc_simd
  ${CMAKE_SOURCE_DIR}/src/test/test_utf8proc_simd.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
)
target_link_libraries(test_utf8proc_simd PRIVATE wisp)

add_wisp_test(test_stream_simd
  ${CMAKE_SOURCE_DIR}/src/test/test_stream_simd.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
)
target_link_libraries(test_stream_simd PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_js_main
  ${CMAKE_SOURCE_DIR}/src/test/test_js_main.c
  ${CMAKE_SOURCE_DIR}/src/processes/js/main.c
)
add_dependencies(test_js_main quickjs_bindings)
target_include_directories(test_js_main PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/src/content/handlers/javascript/quickjs
  ${CMAKE_SOURCE_DIR}/src
)
target_compile_definitions(test_js_main PRIVATE WISP_JS_TESTING)
target_link_libraries(test_js_main PRIVATE qjs nsutils wisp)
add_wisp_test(test_utf8proc_wrapper
  ${CMAKE_SOURCE_DIR}/src/test/test_utf8proc_wrapper.c
)
target_link_libraries(test_utf8proc_wrapper PRIVATE wisp)

add_wisp_test(test_css_simd
  ${CMAKE_SOURCE_DIR}/src/test/test_css_simd.c
)
target_link_libraries(test_css_simd PRIVATE wisp)


add_wisp_test(test_utf8
  ${CMAKE_SOURCE_DIR}/src/test/test_utf8.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
)
target_link_libraries(test_utf8 PRIVATE wisp)
add_wisp_test(test_websocket_mask
  ${CMAKE_SOURCE_DIR}/src/test/test_websocket_mask.c
)
target_link_libraries(test_websocket_mask PRIVATE wisp)

add_wisp_test(test_stream_chunk_decoder
  ${CMAKE_SOURCE_DIR}/src/test/test_stream_chunk_decoder.c
)
target_link_libraries(test_stream_chunk_decoder PRIVATE wisp)

add_wisp_test(test_dirty_grid
  ${CMAKE_SOURCE_DIR}/src/test/test_dirty_grid.c
)
target_link_libraries(test_dirty_grid PRIVATE wisp)
endif()
add_wisp_test(test_ipc_spawn
  ${CMAKE_SOURCE_DIR}/src/test/test_ipc_spawn.c
)
target_link_libraries(test_ipc_spawn PRIVATE wisp)

add_executable(dummy_ipc_target ${CMAKE_SOURCE_DIR}/src/test/dummy_ipc_target.c)
target_link_libraries(dummy_ipc_target PRIVATE wisp)

add_wisp_test(test_content_factory
  ${CMAKE_SOURCE_DIR}/src/test/test_content_factory.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
)
target_link_libraries(test_content_factory PRIVATE wisp)

add_wisp_test(test_task_queue
  ${CMAKE_SOURCE_DIR}/src/test/test_task_queue.c
)
target_link_libraries(test_task_queue PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_shm_dom
  ${CMAKE_SOURCE_DIR}/src/test/test_shm_dom.c
)
target_link_libraries(test_shm_dom PRIVATE wisp ${CHECK_LIBRARIES})
add_wisp_test(test_arena
  ${CMAKE_SOURCE_DIR}/src/test/test_arena.c
)
target_link_libraries(test_arena PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_fetch
  ${CMAKE_SOURCE_DIR}/src/test/test_fetch.c
)
target_link_libraries(test_fetch PRIVATE wisp)
add_wisp_test(test_thread_pool
  ${CMAKE_SOURCE_DIR}/src/test/test_thread_pool.c
)
target_link_libraries(test_thread_pool PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_file
  ${CMAKE_SOURCE_DIR}/src/test/test_file.c
)
target_link_libraries(test_file PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_nscolour
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/test_nscolour.c
)
target_link_libraries(test_nscolour PRIVATE wisp ${CHECK_LIBRARIES})
add_wisp_test(test_generics test_generics.c)
target_link_libraries(test_generics PRIVATE wisp ${CHECK_LIBRARIES})
add_wisp_test(test_content_disposition test_content_disposition.c)
target_link_libraries(test_content_disposition PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_ssl_certs
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/test_ssl_certs.c
)
target_link_libraries(test_ssl_certs PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_browser_window
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/test_browser_window.c
)
target_link_libraries(test_browser_window PRIVATE wisp ${CHECK_LIBRARIES})
add_wisp_test(test_libdom
  ${CMAKE_SOURCE_DIR}/src/test/test_libdom.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
)
target_link_libraries(test_libdom PRIVATE wisp ${CHECK_LIBRARIES})
add_wisp_test(test_challenge test_challenge.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
)
target_link_libraries(test_challenge PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_www_authenticate test_www_authenticate.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c)
target_link_libraries(test_www_authenticate PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_messages
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/test_messages.c
)
target_link_libraries(test_messages PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_talloc
  ${CMAKE_SOURCE_DIR}/src/test/log.c
  ${CMAKE_SOURCE_DIR}/src/test/test_talloc.c
)
target_link_libraries(test_talloc PRIVATE wisp ${CHECK_LIBRARIES})
add_wisp_test(test_content_type test_content_type.c)
target_link_libraries(test_content_type PRIVATE wisp ${CHECK_LIBRARIES})
add_wisp_test(content_disposition content_disposition.c)
target_link_libraries(content_disposition PRIVATE wisp ${CHECK_LIBRARIES})
add_wisp_test(test_cache_control
  ${CMAKE_SOURCE_DIR}/src/test/test_cache_control.c
)
target_link_libraries(test_cache_control PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_strict_transport_security
  ${CMAKE_SOURCE_DIR}/src/test/test_strict_transport_security.c
)
target_link_libraries(test_strict_transport_security PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_log
  ${CMAKE_SOURCE_DIR}/src/test/test_log.c
)
target_compile_definitions(test_log PRIVATE WISP_DISABLE_LOGGING=0)
target_link_libraries(test_log PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_ipc
  ${CMAKE_SOURCE_DIR}/src/test/test_ipc.c
)
target_link_libraries(test_ipc PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_parameter
  ${CMAKE_SOURCE_DIR}/src/test/test_parameter.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
)
target_link_libraries(test_parameter PRIVATE wisp ${CHECK_LIBRARIES})
add_wisp_test(test_network_main test_network_main.c)
target_link_libraries(test_network_main PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_filename
  ${CMAKE_SOURCE_DIR}/src/test/test_filename.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
)
target_link_libraries(test_filename PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(hlcache_test
  ${CMAKE_SOURCE_DIR}/src/test/hlcache_test.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
)
target_link_libraries(hlcache_test PRIVATE wisp ${CHECK_LIBRARIES})

add_wisp_test(test_llcache
  ${CMAKE_SOURCE_DIR}/src/test/llcache.c
  ${CMAKE_SOURCE_DIR}/src/test/log.c
)
target_link_libraries(test_llcache PRIVATE wisp)
