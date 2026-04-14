function(feature_option name description default)

  string(CONCAT desc "${decription} (default: ${default})")

  option(${name} "${desc}" "${default}")

  add_feature_info(${name} ${name} "${desc}")

endfunction()

function(feature_option_environment name description environment_variable_name default)

  if(DEFINED ENV{${environment_variable_name}})
    if("$ENV{${environment_variable_name}}")
      set(defaultValue ON)
    else()
      set(defaultValue OFF)
    endif()
  else()
    set(defaultValue "${default}")
  endif()

  feature_option(${name} "${description}" "${defaultValue}")

endfunction()

include(CMakeDependentOption)

macro(feature_option_dependent name description default depends force)

  string(CONCAT desc
    "${description} (default: ${default}; depends on condition: ${depends})")
  
  cmake_dependent_option(${name} "${desc}" "${default}" "${depends}" "${force}")

  add_feature_info(${name} ${name} "${desc}")

endmacro()

macro(cest_dependent_string_option option strings doc default depends force)

  set(${option}_AVAILABLE YES)

  foreach(depend ${depends})
    cmake_language(EVAL CODE "
      if($depend)
      else()
        set(${option}_AVAILABLE NO)
      endif()"
    )
  endforeach()

  if(${option}_AVAILABLE)
    if(DEFINED CACHE{${option}})
      set(${option} "${${option}}" CACHE STRING "${doc}" FORCE)
    else()
      set(${option} "${default}" CACHE STRING "${doc}" FORCE)
    endif()

    set_property(CACHE ${option} PROPERTY STRINGS ${strings})
  else()
    if(DEFINED CACHE{${option}})
      set(${option} "${${option}}" CACHE INTERNAL "${doc}")
    endif()

    set(${option} "${force}")
  endif()

  unset(${option}_AVAILABLE)

endmacro()

macro(feature_string_option_dependent name strings description default depends force)

  set(allowedValues)
  string(JOIN ", " allowedValues ${strings})
  string(CONCAT desc
    "${description} (allowed values: ${allowedValues}; default: ${default}; \
    depends on condition: ${depends})")
  
  cest_dependent_string_option(
    ${name} "${strings}" "${desc}" "${default}" "${depends}" "${force}"
  )

  add_feature_info(${name} ${name} "${desc}")

  unset(desc)
  unset(allowedValues)

endmacro()
  
