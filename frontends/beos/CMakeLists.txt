
include_directories(${CMAKE_SOURCE_DIR}/include)

set(WISP_BEOS_SRCS
    about.cpp
    audio.cpp
    bitmap.cpp
    cookies.cpp
    download.cpp
    fetch_rsrc.cpp
    filetype.cpp
    font.cpp
    gui.cpp
    gui_options.cpp
    login.cpp
    plotters.cpp
    scaffolding.cpp
    schedule.cpp
    search.cpp
    throbber.cpp
    window.cpp
)

set(MESSAGES_LANGUAGES en fr de it nl zh_CN)
set(MESSAGES_PLATFORMS beos all)
set(FAT_MESSAGES "${CMAKE_SOURCE_DIR}/src/resources/FatMessages")
set(SPLIT_MESSAGES_SCRIPT "${CMAKE_SOURCE_DIR}/src/utils/split_messages.py")
set(GENERATED_MESSAGES_DIR "${CMAKE_CURRENT_BINARY_DIR}/res")

set(GENERATED_MESSAGES_FILES)

foreach(LANG ${MESSAGES_LANGUAGES})
    set(MSG_FILE "${GENERATED_MESSAGES_DIR}/${LANG}/Messages")
    list(APPEND GENERATED_MESSAGES_FILES ${MSG_FILE})

    add_custom_command(
        OUTPUT ${MSG_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GENERATED_MESSAGES_DIR}/${LANG}"
        COMMAND Python3::Interpreter ${SPLIT_MESSAGES_SCRIPT} ${FAT_MESSAGES} ${MSG_FILE} ${LANG} ${MESSAGES_PLATFORMS}
        DEPENDS ${FAT_MESSAGES} ${SPLIT_MESSAGES_SCRIPT}
        COMMENT "Generating Messages for ${LANG}"
    )
endforeach()

add_custom_target(beos_messages DEPENDS ${GENERATED_MESSAGES_FILES})

add_executable(wisp-beos
    ${WISP_BEOS_SRCS}
)

add_dependencies(wisp-beos beos_messages)
target_include_directories(wisp-beos PRIVATE ${GENERATED_MESSAGES_DIR} ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(wisp-beos wisp translation tracker network media columnlistview game localestub be)

target_compile_definitions(wisp-beos PRIVATE nsbeos)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_INSTALL_DO_STRIP OFF)
endif()

install(TARGETS wisp-beos DESTINATION bin)
