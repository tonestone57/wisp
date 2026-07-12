find_package(PkgConfig)

# Second GTK_RESPATH entry is for AppImages
add_definitions(-DGTK_RESPATH="${CMAKE_INSTALL_PREFIX}/share/wisp-gtk:./${CMAKE_INSTALL_PREFIX}/share/wisp-gtk" -DWITH_GRESOURCE -DWITH_BUILTIN_PIXBUF -DWISP_HOMEPAGE="about:welcome")

pkg_check_modules(GTK REQUIRED gtk+-3.0>=3.24)
include_directories(${GTK_INCLUDE_DIRS})

pkg_check_modules(PIPEWIRE libpipewire-0.3)
if(PIPEWIRE_FOUND)
	add_definitions(-DWITH_PIPEWIRE)
	include_directories(${PIPEWIRE_INCLUDE_DIRS})
	set(GTK_AUDIO_LIBRARIES ${PIPEWIRE_LIBRARIES})
	message(STATUS "GTK Frontend: Audio enabled via PipeWire")
else()
	pkg_check_modules(PULSE pulse-simple pulse)
	if(PULSE_FOUND)
		add_definitions(-DWITH_PULSE)
		include_directories(${PULSE_INCLUDE_DIRS})
		set(GTK_AUDIO_LIBRARIES ${PULSE_LIBRARIES})
		message(STATUS "GTK Frontend: Audio enabled via PulseAudio")
	else()
		message(WARNING "GTK Frontend: Neither PipeWire nor PulseAudio found! Audio disabled.")
		set(GTK_AUDIO_LIBRARIES "")
	endif()
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

# Copy default.css to resources to avoid symlinks
configure_file(
	${CMAKE_SOURCE_DIR}/src/resources/default.css
	${CMAKE_CURRENT_BINARY_DIR}/res/default.css
	COPYONLY
)

add_executable(wisp-gtk
	gui.c
	misc.c
	schedule.c
	layout_pango.c
	bitmap.c
	plotters.c
	scaffolding.c
	gdk.c
	completion.c
	throbber.c
	accelerator.c
	selection.c
	window.c
	fetch.c
	download.c
	menu.c
	print.c
	search.c
	tabs.c
	toolbar.c
	compat.c
	viewdata.c
	viewsource.c
	preferences.c
	about.c
	resources.c
	corewindow.c
	local_history.c
	global_history.c
	cookies.c
	hotlist.c
	page_info.c
	audio.c
)

target_link_libraries(wisp-gtk wisp ${MATH_LIB} ${WISP_COMMON_LIBS} ${GTK_LIBRARIES} ${GTK_AUDIO_LIBRARIES})

install(TARGETS wisp-gtk DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY res/ DESTINATION ${CMAKE_INSTALL_DATADIR}/wisp-gtk)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/res/default.css DESTINATION ${CMAKE_INSTALL_DATADIR}/wisp-gtk)
