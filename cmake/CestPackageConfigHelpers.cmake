function(cest_get_target_configurations out_target_configuations)

  get_filename_component(targetsDir ${CMAKE_PARENT_LIST_FILE} DIRECTORY)

  file(GLOB targets "${targetsDir}/${PACKAGE_FIND_NAME}[Tt]argets-*.cmake")

  set(configurations)

  foreach(target ${targets})
    string(REGEX MATCH "${PACKAGE_FIND_NAME}[Tt]argets-([a-zA-Z]+).cmake$"
           out "${target}"
    )

    if(NOT CMAKE_MATCH_1)
      continue()
    endif()

    string(TOLOWER ${CMAKE_MATCH_1} configuration)
    list(APPEND configurations ${configuration})
  endforeach()

  set(CEST_${CMAKE_FIND_PACKAGE_NAME}_FOUND_CONFIGURATIONS "${configurations}"
      CACHE INTERNAL
      "Found all installed target configurations for single-configuration \
      installations.")

  set(${out_target_configuations} ${configurations} PARENT_SCOPE)

endfunction()

function(cest_is_debug_only_config out_is_debug_only_config target_configurations)

  list(LENGTH target_configurations count)
  set(result FALSE)

  if(count EQUAL 1)
    list(GET target_configurations 0 configuration)
    string(TOLOWER ${configuration} configuration)

    if(configuration STREQUAL "debug")
      set(result TRUE)
    endif()
  endif()

  set(${out_is_debug_only_config} ${result} PARENT_SCOPE)

endfunction()

function(cest_printable_configurations out_configurations configurations)

  set(result)

  foreach(configuration ${configurations})
    string(SUBSTRING ${configuration} 0 1 firstLetter)
    string(TOUPPER ${firstLetter} firstLetter)

    string(REGEX REPLACE "^.(.*)" "${firstLetter}\\1"
          configuration ${configuration}
    )

    list(APPEND result ${configuration})
  endforeach()

  string(JOIN "," result ${result})

  set(${out_configurations} ${result} PARENT_SCOPE)

endfunction()

function(cest_to_bool out_variable value)

  if(value)
    set(${out_variable} TRUE PARENT_SCOPE)
  else()
    set(${out_variable} FALSE PARENT_SCOPE)
  endif()

endfunction()

function(cest_build_type_requirements_install_tree
        out_package_version out_package_version_unsuitable
        cvf_is_multi_config cvf_is_vcpkg
)
  cest_get_target_configurations(cvfTargetConfigurations)

  message(DEBUG "cvfTargetConfigurations = ${cvfTargetConfigurations}")
  message(DEBUG "cvf_is_multi_config = ${cvf_is_multi_config}")
  message(DEBUG "cvf_is_vcpkg = ${cvf_is_vcpkg}")
  message(DEBUG "CMAKE_CURRENT_LIST_FILE = ${CMAKE_CURRENT_LIST_FILE}")

  if(NOT cvf_is_multi_config AND NOT cvf_is_vcpkg)
    cest_is_debug_only_config(cvfIsDebugOnly "${cvfTargetConfigurations}")

    list(LENGTH cvfTargetConfigurations count)

    get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    cest_to_bool(isMultiConfig ${isMultiConfig}) # Don't quote, must fail if undefined

    string(TOLOWER "${CMAKE_BUILD_TYPE}" cmakeBuildTypeLower)

    message(DEBUG "isMultiConfig = ${isMultiConfig}")
    message(DEBUG "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
    message(DEBUG "cvfIsDebugOnly = ${cvfIsDebugOnly}")
    message(DEBUG "MSVC = ${MSVC}")

    if(isMultiConfig)
      set(${out_package_version} "${${out_package_version}} single-config"
          PARENT_SCOPE)
      set(${out_package_version_unsuitable} TRUE PARENT_SCOPE)
      return()
    elseif(count EQUAL 0)
      set(${out_package_version} "${${out_package_version}} no-configuration"
          PARENT_SCOPE)
      set(${out_package_version_unsuitable} TRUE PARENT_SCOPE)
      return()
    elseif(MSVC AND ((cmakeBuildTypeLower STREQUAL "debug" AND
           NOT "debug" IN_LIST cvfTargetConfigurations) OR
          (NOT cmakeBuildTypeLower STREQUAL "debug" AND cvfIsDebugOnly))
    )
      cest_printable_configurations(cestPrintableConfigurations
        "${cvfTargetConfigurations}"
      )

      set(${out_package_version}
          "${${out_package_version}} single-config [${cestPrintableConfigurations}]"
          PARENT_SCOPE)
      set(${out_package_version_unsuitable} TRUE PARENT_SCOPE)
      return()
    endif()
  endif()

endfunction()

function(cest_build_type_requirements_build_tree
        out_package_version out_package_version_unsuitable
        cvf_is_multi_config cvf_config_build_type cvf_match_buildtree
)
  message(DEBUG "cvf_is_multi_config = ${cvf_is_multi_config}")
  message(DEBUG "CMAKE_CURRENT_LIST_FILE = ${CMAKE_CURRENT_LIST_FILE}")

  if(NOT cvf_is_multi_config)
    get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    cest_to_bool(isMultiConfig ${isMultiConfig})

    string(TOLOWER "${CMAKE_BUILD_TYPE}" cmakeBuildTypeLower)
    string(TOLOWER "${cvf_config_build_type}" cvfConfigBuildTypeLower)

    message(DEBUG "isMultiConfig = ${isMultiConfig}")
    message(DEBUG "cvf_match_buildtree = ${cvf_match_buildtree}")
    message(DEBUG "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
    message(DEBUG "cvf_config_build_type = ${cvf_config_build_type}")
    message(DEBUG "MSVC = ${MSVC}")

    if(isMultiConfig)
      set(${out_package_version} "${${out_package_version}} single-config"
          PARENT_SCOPE)
      set(${out_package_version_unsuitable} TRUE PARENT_SCOPE)
      return()
    elseif((cvf_match_buildtree AND
          NOT cmakeBuildTypeLower STREQUAL cvfConfigBuildTypeLower) OR
          (MSVC AND ((cmakeBuildTypeLower STREQUAL "debug" AND
          NOT cvfConfigBuildTypeLower STREQUAL "debug") OR
          (NOT cmakeBuildTypeLower STREQUAL "debug" AND
          cvfConfigBuildTypeLower STREQUAL "debug")))
    )
      set(${out_package_version}
          "${${out_package_version}} single-config CMAKE_BUILD_TYPE=${cvf_config_build_type}"
          PARENT_SCOPE)
      set(${out_package_version_unsuitable} TRUE PARENT_SCOPE)
      return()
    endif()
  endif()

endfunction()

function(cest_get_build_types out_build_types cvf_is_multi_config cvf_is_vcpkg)

  cest_printable_configurations(cestPrintableConfigurations
    "${CEST_${CMAKE_FIND_PACKAGE_NAME}_FOUND_CONFIGURATIONS}"
  )

  if(cvf_is_vcpkg)
    set(cest_build_type " Vcpkg[${cestPrintableConfigurations}]")
  elseif(cvf_is_multi_config)
    set(cest_build_type " Multi-Config[${cestPrintableConfigurations}]")
  else()
    set(cest_build_type " [${cestPrintableConfigurations}]")
  endif()

  set(${out_build_types} ${cest_build_type} PARENT_SCOPE)

endfunction()