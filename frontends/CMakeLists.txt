if(WISP_WERROR)
	if(MSVC)
		add_compile_options($<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:/WX>)
	else()
		add_compile_options($<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-Werror>
                            $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-Werror=attribute-warning>)
	endif()
endif()

include_directories("${CMAKE_SOURCE_DIR}/frontends")

if(WISP_BUILD_GTK_FRONTEND)
    add_subdirectory(gtk)
endif()

if(WISP_BUILD_VI_FRONTEND)
    add_subdirectory(visurf)
endif()

if(WISP_BUILD_WINDOWS_FRONTEND)
    add_subdirectory(windows)
endif()

if(WISP_BUILD_QT_FRONTEND)
    add_subdirectory(qt)
endif()

if(WISP_BUILD_MONKEY_FRONTEND)
    add_subdirectory(monkey)
endif()

if(WISP_BUILD_MACOS_FRONTEND)
    add_subdirectory(macos)
endif()

if(WISP_BUILD_BEOS_FRONTEND)
    add_subdirectory(beos)
endif()
