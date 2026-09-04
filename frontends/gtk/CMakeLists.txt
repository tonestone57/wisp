find_package(PkgConfig)

# Second GTK_RESPATH entry is for AppImages
add_definitions(-DGTK_RESPATH="${CMAKE_INSTALL_PREFIX}/share/wisp-gtk:./${CMAKE_INSTALL_PREFIX}/share/wisp-gtk" -DWITH_GRESOURCE -DWISP_HOMEPAGE="about:welcome")

pkg_check_modules(GTK REQUIRED gtk+-3.0>=3.2)
include_directories(${GTK_INCLUDE_DIRS})

pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
include_directories(${FONTCONFIG_INCLUDE_DIRS})

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

# Clean build resources folder to avoid symlink duplication/overwriting conflicts
file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/res)

# Copy res/ directory from source to binary directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Delete the symlinks from the copied folder in build directory so they can be replaced by real files/directories
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/res/adblock.css)
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/res/credits.html)
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/res/internal.css)
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/res/license.html)
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/res/welcome.html)
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/res/user.css)
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/res/SearchEngines)
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/res/icons)

# Copy common resources to binary directory to replace symlinks
configure_file(
	${CMAKE_SOURCE_DIR}/src/resources/default.css
	${CMAKE_CURRENT_BINARY_DIR}/res/default.css
	COPYONLY
)
configure_file(
	${CMAKE_SOURCE_DIR}/src/resources/quirks.css
	${CMAKE_CURRENT_BINARY_DIR}/res/quirks.css
	COPYONLY
)
configure_file(
	${CMAKE_SOURCE_DIR}/src/resources/adblock.css
	${CMAKE_CURRENT_BINARY_DIR}/res/adblock.css
	COPYONLY
)
configure_file(
	${CMAKE_SOURCE_DIR}/src/resources/credits.html
	${CMAKE_CURRENT_BINARY_DIR}/res/credits.html
	COPYONLY
)
configure_file(
	${CMAKE_SOURCE_DIR}/src/resources/internal.css
	${CMAKE_CURRENT_BINARY_DIR}/res/internal.css
	COPYONLY
)
configure_file(
	${CMAKE_SOURCE_DIR}/src/resources/license.html
	${CMAKE_CURRENT_BINARY_DIR}/res/license.html
	COPYONLY
)
configure_file(
	${CMAKE_SOURCE_DIR}/src/resources/en/welcome.html
	${CMAKE_CURRENT_BINARY_DIR}/res/welcome.html
	COPYONLY
)
configure_file(
	${CMAKE_SOURCE_DIR}/src/resources/user-win10.css
	${CMAKE_CURRENT_BINARY_DIR}/res/user.css
	COPYONLY
)
configure_file(
	${CMAKE_SOURCE_DIR}/src/resources/SearchEngines
	${CMAKE_CURRENT_BINARY_DIR}/res/SearchEngines
	COPYONLY
)

# Copy the icons directory to replace the icons symlink
file(COPY ${CMAKE_SOURCE_DIR}/src/resources/icons DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/res)

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
	find.c
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

target_link_libraries(wisp-gtk wisp ${MATH_LIB} ${WISP_COMMON_LIBS} ${GTK_LIBRARIES} ${GTK_AUDIO_LIBRARIES} ${FONTCONFIG_LIBRARIES})

install(TARGETS wisp-gtk DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/res/ DESTINATION ${CMAKE_INSTALL_DATADIR}/wisp-gtk)
