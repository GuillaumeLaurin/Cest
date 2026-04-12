function(cest_invert_bool out_variable value) 

  if(value)
    set(${out_variable} FALSE PARENT_SCOPE)
  else()
    set(${out_variable} TRUE PARENT_SCOPE)
  endif()

endfunction()

function(cest_to_bool out_variable value)

  if(value)
    set(${out_variable} TRUE PARENT_SCOPE)
  else()
    set(${out_variable} FALSE PARENT_SCOPE)
  endif()

endfunction()

macro(cest_find_package package_name)

  find_package(${package_name} ${ARGN})

  if(${package_name}_FOUND)
    set(args "${package_name}")
    # These arguments will be forwarded to the find_package() by find_dependency()
    list(APPEND args "${ARGN}")
    list(REMOVE_ITEM args "REQUIRED" "QUIET")
    # Remove all empty items
    list(REMOVE_ITEM args "")
    # Convert to the string
    string(REPLACE ";" " " args "${args}")

    # Check if the given args are in the CEST_PACKAGE_DEPENDENCIES list
    get_property(packageDependencies GLOBAL PROPERTY CEST_PACKAGE_DEPENDENCIES)

    if(NOT args IN_LIST packageDependencies)
      set_property(GLOBAL APPEND PROPERTY CEST_PACKAGE_DEPENDENCIES "${args}")
    endif()
  endif()

  unset(args)

endmacro()

function(cest_generate_find_dependency_calls out_dependency_calls)

  set(findDependencyCalls)

  string(REGEX REPLACE "([^;]+)" "find_dependency(\\1)" findDependencyCalls
    "${packageDependencies}")
  
  string(REPLACE ";" "\n" findDependencyCalls "${findDependencyCalls}")

  set(${out_dependency_calls} ${findDependencyCalls} PARENT_SCOPE)

endfunction()

function(target_optional_compile_definitions target scope)

  set(options ADVANCED FEATURE)
  set(oneValueArgs NAME DESCRIPTION DEFAULT)
  set(multiValueArgs ENABLED DISABLED)
  cmake_parse_arguments(PARSE_ARGV 2 CEST "${options}" "${oneValueArgs}"
    "${multiValueArgs}"
  )

  if(DEFINED CEST_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "The ${CMAKE_CURRENT_FUNCTION}() was passed extra arguments: \
            ${CEST_UNPARSED_ARGUMENTS}")
  endif()

  option(${CEST_NAME} "${CEST_DESCRIPTION}" ${CEST_DEFAULT})

  if(${${CEST_NAME}})
    target_compile_definitions(${target} ${scope} ${CEST_ENABLED})
  else()
    target_compile_definitions(${target} ${scope} ${CEST_DISABLED})
  endif()

  if(CEST_FEATURE)
    add_feature_info(${CEST_NAME} ${CEST_NAME} "${CEST_DESCRIPTION}")
  endif()

  if(CEST_ADVANCED)
    mark_as_advanced(${CEST_NAME})
  endif()

endfunction()

function(cest_set_lto_compile_definition target)

    get_target_property(cestHasLTO ${target} INTERPROCEDURAL_OPTIMIZATION)

    target_compile_definitions(${target} PRIVATE -DCEST_LTO=${cestHasLTO})

endfunction()

function(cest_read_version out_version out_major out_minor out_patch out_tweak)

  # Arguments
  set(oneValueArgs VERSION_HEADER PREFIX HEADER_FOR)
  cmake_parse_arguments(PARSE_ARGV 5 CEST "" "${oneValueArgs}" "")

  if(DEFINED CEST_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "The ${CMAKE_CURRENT_FUNCTION}() was passed extra arguments: \
            ${CEST_UNPARSED_ARGUMENTS}")
  endif()

  # Debug setup
  list(APPEND CMAKE_MESSAGE_CONTEXT VersionHeader)
  set(mainMessage "Reading Version Header for ${CEST_HEADER_FOR}")
  message(DEBUG ${mainMessage})
  list(APPEND CMAKE_MESSAGE_INDENT "  ")

  # ---

  file(STRINGS ${CEST_VERSION_HEADER} versionFileContent
       REGEX "^#define ${CEST_PREFIX}.*_VERSION_[A-Z]+ +[0-9]+"
  )

  message(DEBUG "Version file content - ${versionFileContent}")

  set(regex ".+_MAJOR +([0-9]+);.+_MINOR +([0-9]+);.+_BUGFIX +([0-9]+);.+_BUILD +([0-9]+)")
  string(REGEX MATCHALL "${regex}" match "${versionFileContent}")

  if(NOT match)
    message(FATAL_ERROR "Could not detect project version number \
            from ${CEST_VERSION_HEADER} in ${CMAKE_CURRENT_FUNCTION}().")
  endif()

  message(DEBUG "Matched version string - ${match}")

  set(version "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}.${CMAKE_MATCH_4}")

  # ---

  message(DEBUG "${out_version} - ${version}")
  message(DEBUG "${out_major} - ${CMAKE_MATCH_1}")
  message(DEBUG "${out_minor} - ${CMAKE_MATCH_2}")
  message(DEBUG "${out_patch} - ${CMAKE_MATCH_3}")
  message(DEBUG "${out_tweak} - ${CMAKE_MATCH_4}")

  # Debug finish
  list(POP_BACK CMAKE_MESSAGE_INDENT)
  message(DEBUG "${mainMessage} - done")
  list(POP_BACK CMAKE_MESSAGE_CONTEXT)

  # Return values
  set(${out_version} ${version} PARENT_SCOPE)
  set(${out_major} ${CMAKE_MATCH_1} PARENT_SCOPE)
  set(${out_minor} ${CMAKE_MATCH_2} PARENT_SCOPE)
  set(${out_patch} ${CMAKE_MATCH_3} PARENT_SCOPE)
  set(${out_tweak} ${CMAKE_MATCH_4} PARENT_SCOPE)

endfunction()

macro(cest_set_rc_flags)

  # Remove RC flags from the previous call
  if(NOT CEST_RC_FLAGS_BACKUP STREQUAL "")
    foreach(toRemove ${CEST_RC_FLAGS_BACKUP})
      string(REGEX REPLACE "${toRemove}" "" CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS}")
    endforeach()
    unset(toRemove)
  endif()

  list(APPEND CMAKE_RC_FLAGS " ${ARGN}")
  list(JOIN CMAKE_RC_FLAGS " " CMAKE_RC_FLAGS)

  # Remove redundant whitespaces
  string(REGEX REPLACE " +" " " CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS}")
  string(STRIP "${CMAKE_RC_FLAGS}" CMAKE_RC_FLAGS)

  # Will be removed from the CMAKE_RC_FLAGS in a future call
  set(CEST_RC_FLAGS_BACKUP "${ARGN}")

endmacro()

function(cest_print_linking_against target)

  if(CEST_IS_MULTI_CONFIG OR CEST_BUILD_TYPE_UPPER STREQUAL "")
      return()
  endif()

  if(WIN32 AND BUILD_SHARED_LIBS)
    get_target_property(libraryFilepath ${target} IMPORTED_IMPLIB_${CEST_BUILD_TYPE_UPPER})
  else()
    get_target_property(libraryFilepath ${target} IMPORTED_LOCATION_${CEST_BUILD_TYPE_UPPER})
  endif()

  message(VERBOSE "Linking against ${target} at ${libraryFilepath}")

endfunction()

function(cest_is_ccache_compiler_launcher out_variable)

  if(NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
    set(${out_variable} FALSE PARENT_SCOPE)
    return()
  endif()

  cmake_path(GET CMAKE_CXX_COMPILER_LAUNCHER STEM ccacheStem)
  if(NOT ccacheStem STREQUAL "ccache" AND NOT ccacheStem STREQUAL "sccache")
    set(${out_variable} FALSE PARENT_SCOPE)
    return()
  endif()

  set(${out_variable} TRUE PARENT_SCOPE)

endfunction()

function(cest_should_fix_ccache_msvc out_variable)

  if(NOT WIN32 OR NOT MSVC OR MINGW OR NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
    set(${out_variable} FALSE PARENT_SCOPE)
    return()
  endif()

  set(isCcacheCompilerLauncher FALSE)
  cest_is_ccache_compiler_launcher(isCcacheCompilerLauncher)

  set(${out_variable} ${isCcacheCompilerLauncher} PARENT_SCOPE)

endfunction()

function(cest_should_disable_precompile_headers out_variable)

  if(DEFINED CEST_CCACHE_VERSION AND NOT CEST_CCACHE_VERSION STREQUAL "")
    if(CEST_CCACHE_VERSION VERSION_GREATER_EQUAL "4.10" OR
       CEST_CCACHE_VERSION STREQUAL "git-ref"
    )
      set(${out_variable} FALSE PARENT_SCOPE)
    else()
      set(${out_variable} TRUE PARENT_SCOPE)
    endif()
    return()
  endif()

  set(helpString "Ccache version used to determine whether to disable PCH (MSVC only).")

  execute_process(
    COMMAND "${CMAKE_CXX_COMPILER_LAUNCHER}" --print-version
    RESULT_VARIABLE exitCode
    OUTPUT_VARIABLE ccacheVersionRaw
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )

  if(exitCode STREQUAL "no such file or directory")
    set(${out_variable} FALSE PARENT_SCOPE)
    return()
  endif()

  if(NOT exitCode EQUAL 0)
    set(CEST_CCACHE_VERSION "0" CACHE INTERNAL "${helpString}")
    set(${out_variable} TRUE PARENT_SCOPE)
    return()
  endif()

  set(regexpGitRef
      "^.*\.[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f]*$")

  if(ccacheVersionRaw MATCHES "${regexpGitRef}")
    set(CEST_CCACHE_VERSION "git-ref" CACHE INTERNAL "${helpString}")
    set(${out_variable} FALSE PARENT_SCOPE)
    return()
  endif()

  set(regexpVersion "^[0-9]+\.[0-9]+(\.[0-9]+)?(\.[0-9]+)?$")

  if(NOT ccacheVersionRaw MATCHES "${regexpVersion}")
    message(FATAL_ERROR "Parsing of the 'ccache --print-version' failed \
            in ${CMAKE_CURRENT_FUNCTION}().")
  endif()

  set(CEST_CCACHE_VERSION "${CMAKE_MATCH_0}" CACHE INTERNAL "${helpString}")

  if(CEST_CCACHE_VERSION VERSION_GREATER_EQUAL "4.10")
    set(${out_variable} FALSE PARENT_SCOPE)
  else()
    set(${out_variable} TRUE PARENT_SCOPE)
  endif()

endfunction()

function(cest_fix_ccache_msvc_325)

  get_property(help_string CACHE CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
    PROPERTY HELPSTRING
  )
  if(NOT help_string)
    set(help_string "Default value for MSVC_DEBUG_INFORMATION_FORMAT of targets.")
  endif()

  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
    "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>"
    CACHE BOOL ${help_string} FORCE
  )

  mark_as_advanced(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT)

endfunction()

function(cest_replace_Zi_by_Z7_for option help_string)

  if(DEFINED ${option} AND ${option} MATCHES "(/|-)(Zi|ZI)")
    string(REGEX REPLACE "(/|-)(Zi|ZI)" "/Z7" ${option} "${${option}}")

    get_property(help_string_property CACHE ${option} PROPERTY HELPSTRING)
    if(NOT help_string_property)
      set(help_string_property ${help_string})
    endif()

    set(${option} ${${option}} CACHE STRING ${help_string_property} FORCE)
  endif()

endfunction()

function(cest_fix_ccache_msvc_324)

  if(CEST_IS_MULTI_CONFIG OR CEST_BUILD_TYPE_LOWER STREQUAL "")
    message(STATUS "The ccache compiler launcher is not supported for multi-configuration \
            generators or with undefined CMAKE_BUILD_TYPE on CMake <3.25")
    return()
  endif()

  if(CEST_BUILD_TYPE_LOWER STREQUAL "debug")
    cest_replace_Zi_by_Z7_for(CMAKE_CXX_FLAGS_DEBUG
      "Flags used by the CXX compiler during DEBUG builds.")
    cest_replace_Zi_by_Z7_for(CMAKE_C_FLAGS_DEBUG
      "Flags used by the C compiler during DEBUG builds.")
  elseif(CEST_BUILD_TYPE_LOWER STREQUAL "release")
    cest_replace_Zi_by_Z7_for(CMAKE_CXX_FLAGS_RELEASE
      "Flags used by the CXX compiler during RELEASE builds.")
    cest_replace_Zi_by_Z7_for(CMAKE_C_FLAGS_RELEASE
      "Flags used by the C compiler during RELEASE builds.")
  elseif(CEST_BUILD_TYPE_LOWER STREQUAL "relwithdebinfo")
    cest_replace_Zi_by_Z7_for(CMAKE_CXX_FLAGS_RELWITHDEBINFO
      "Flags used by the CXX compiler during RELWITHDEBINFO builds.")
    cest_replace_Zi_by_Z7_for(CMAKE_C_FLAGS_RELWITHDEBINFO
      "Flags used by the C compiler during RELWITHDEBINFO builds.")
  endif()

endfunction()

function(cest_fix_ccache_msvc)
  
  set(isNewMsvcDebugInformationFormat FALSE)
  cest_is_new_msvc_debug_information_format_325(isNewMsvcDebugInformationFormat)

  if(isNewMsvcDebugInformationFormat)
    cest_fix_ccache_msvc_325()
  else()
    cest_fix_ccache_msvc_324()
  endif()

  if(isNewMsvcDebugInformationFormat OR NOT CEST_IS_MULTI_CONFIG)
    cest_disable_precompile_headers()
  endif()

endfunction()

function(cest_should_fix_ccache_clang out_variable)

  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(${out_variable} FALSE PARENT_SCOPE)
    return()
  endif()

  set(isCcacheCompilerLauncher FALSE)
  cest_is_ccache_compiler_launcher(isCcacheCompilerLauncher)

  set(${out_variable} ${isCcacheCompilerLauncher} PARENT_SCOPE)

endfunction()

function(cest_fix_ccache)

  set(shouldFixCcacheMsvc FALSE)
  cest_should_fix_ccache_msvc(shouldFixCcacheMsvc)

  if(shouldFixCcacheMsvc)
    cest_fix_ccache_msvc()
  endif()

  set(shouldFixCcacheClang FALSE)
  cest_should_fix_ccache_clang(shouldFixCcacheClang)

  if(shouldFixCcacheClang)
    list(APPEND CMAKE_CXX_COMPILE_OPTIONS_CREATE_PCH -Xclang -fno-pch-timestamp)

    set(CMAKE_CXX_COMPILE_OPTIONS_CREATE_PCH
        "${CMAKE_CXX_COMPILE_OPTIONS_CREATE_PCH}" PARENT_SCOPE
    )
  endif()

endfunction()

function(cest_set_compatible_interface_string target)

  set(multiValueArgs PROPERTIES)
  cmake_parse_arguments(PARSE_ARGV 1 CEST "" "" "${multiValueArgs}")

  if(DEFINED CEST_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "The ${CMAKE_CURRENT_FUNCTION}() was passed extra arguments: \
            ${CEST_UNPARSED_ARGUMENTS}")
  endif()

  foreach(property ${CEST_PROPERTIES})
    if(NOT (target STREQUAL Cest_target AND property STREQUAL "VERSION_MAJOR"))
      get_target_property(${target}_${property} ${target} ${property})
    endif()

    set_property(
      TARGET ${target}
      PROPERTY INTERFACE_${target}_${property} ${${target}_${property}}
    )

    set_property(
      TARGET ${target}
      APPEND PROPERTY COMPATIBLE_INTERFACE_STRING ${target}_${property}
    )
  endforeach()

endfunction()

function(cest_generate_target_includes out_variable)

  set(includeTmpl "include(\"\${CMAKE_CURRENT_LIST_DIR}/@target@.cmake\")")
  set(includeReplaced)
  set(result)

  string(REPLACE "@target@" "CestTargets" includeReplaced "${includeTmpl}")
  list(APPEND result ${includeReplaced})

  list(JOIN result "\n" result)

  set(${out_variable} ${result} PARENT_SCOPE)

endfunction()