function(cs_system_info)
  include(CMakePrintSystemInformation)
endfunction()

function(cs_verbose_makefile)
  include(CMAKE_VERBOSE_MAKEFILE ON)
endfunction()

function(cs_prints_vars)
  set(exclude_cmake yes)
  if(ARGC GREATER_EQUAL 1 AND DEFINED ARGV0 AND NOT ARGV0)
    set(exclude_cmake no)
    message(STATUS "All variables:")
  else()
    message(STATUS "All non-cmake variables:")
  endif()

  get_cmake_property(variable_names VARIABLES)
  foreach(variable ${variable_names})
    if(exclude_cmake AND variable MATCHES "(^(CMAKE_.*)|^(_.*))")
      continue()
    endif()

    message("${variable}=${${variable}}")
  endforeach()
endfunction()

function(cs_print_env_vars)
  message(STATUS "All environment variables:")
  execute_process(COMMAND "${CMAKE_COMMAND}" "-E" "environment")
endfunction()

if(NOT CMAKE_PROPERTY_LIST)
  execute_process(
    COMMAND cmake --help-property-list
    OUTPUT_VARIABLE CMAKE_PROPERTY_LIST
  )

  string(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
  string(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
endif()

function(cs_print_target_properties target)

  if(NOT TARGET ${target})
    message(FATAL_ERROR "There is no target named: ${target}")
  endif()

  message(STATUS "Target properties for '${target}':")

  string(TOUPPER "${CMAKE_BUILD_TYPE}" cmakeBuildTypeUpper)

  foreach(property ${CMAKE_PROPERTY_LIST})
    string(REPLACE "<CONFIG>" "${cmakeBuildTypeUpper}" property ${property})

    if(property STREQUAL "LOCATION" OR property MATCHES "^LOCATION_" OR
       property MATCHES "_LOCATION$"
    )
      continue()
    endif()

    get_property(isTargetSet TARGET ${target} PROPERTY ${property} SET)

    if(isTargetSet)
      get_target_property(value ${target} ${property})
      message("${property} = ${value}")
    endif()
  endforeach()

endfunction()

function(cs_print_source_properties source)

  if(NOT EXISTS ${source})
    message(FATAL_ERROR "There is no source file named: ${source}")
  endif()

  message(STATUS "Source file properties for ${source}:")

  string(TOUPPER "${CMAKE_BUILD_TYPE}" cmakeBuildTypeUpper)

  foreach(property ${CMAKE_PROPERTY_LIST})
    string(REPLACE "<CONFIG>" "${cmakeBuildTypeUpper}" property ${property})

    if(property STREQUAL "LOCATION" or property MATCHES "^LOCATION_" OR
       property MATCHES "_LOCATION$"
    )
      continue()
    endif()

    get_property(isTargetSet SOURCE "${source}" PROPERTY ${property} SET)

    if(isTargetSet)
      get_source_file_property(value "${source}" ${property})
      message("${property} = ${value}")
    endif()
  endforeach()

endfunction()

function(p variable)
  message("|||-- ${variable} : ${${variable}}")
endfunction()

function(ps variable)
  message(STATUS "${variable} : ${${variable}}")
endfunction()

function(pn variable)
  message(NOTICE "${variable} : ${${variable}}")
endfunction()

function(pb variable)
  if(NOT DEFINED ${variable})
    message("|||-- ${variable} : ${variable}-NOTFOUND")
  elseif(${variable})
    message("|||-- ${variable} : ON")
  else()
    message("|||-- ${variable} : OFF")
  endif()
endfunction()