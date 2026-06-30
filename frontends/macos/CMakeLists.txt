set(MACOS_FRONTEND_SOURCES
    audio.c
    bitmap.m
    fetch.m
    font.m
    main.m
    plot.m
    schedule.c
    window.m
    WispApp.m
    WispView.m
    WispWindow.m
)

add_executable(wisp-macos ${MACOS_FRONTEND_SOURCES})

target_include_directories(wisp-macos PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/frontends
)

target_compile_options(wisp-macos PRIVATE -fobjc-arc)

target_link_libraries(wisp-macos
    PRIVATE
    wisp
    "-framework Cocoa"
    "-framework CoreGraphics"
    "-framework CoreText"
    "-framework AudioToolbox"
    "-framework QuartzCore"
)

set_target_properties(wisp-macos PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.wispbrowser.wisp"
    MACOSX_BUNDLE_BUNDLE_NAME "Wisp"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
)

install(TARGETS wisp-macos BUNDLE DESTINATION .)
