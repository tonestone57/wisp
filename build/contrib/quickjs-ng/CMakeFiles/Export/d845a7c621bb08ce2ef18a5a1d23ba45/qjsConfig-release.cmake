#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "qjs" for configuration "Release"
set_property(TARGET qjs APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(qjs PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libqjs.so.0.11.0"
  IMPORTED_SONAME_RELEASE "libqjs.so.0"
  )

list(APPEND _cmake_import_check_targets qjs )
list(APPEND _cmake_import_check_files_for_qjs "${_IMPORT_PREFIX}/lib/libqjs.so.0.11.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
