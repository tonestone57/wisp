file(REMOVE_RECURSE
  "../quickjs/console.c"
  "../quickjs/window.c"
  "CMakeFiles/quickjs_bindings"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/quickjs_bindings.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
