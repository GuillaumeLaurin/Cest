function(cest_resource_and_manifest target)

  set(options TEST)
  set(oneValueArgs OUTPUT_DIR RESOURCES_DIR RESOURCE_BASENAME MANIFEST_BASENAME)
  cmake_parse_arguments(PARSE_ARGV 1 CEST ${options} "${oneValueArgs}" "")

  if(DEFINED CEST_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "The ${CMAKE_CURRENT_FUNCTION}() was passed extra arguments: \
            ${CEST_UNPARSED_ARGUMENTS}")
  endif()

  get_target_property(target_type ${target} TYPE)

  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" OR
     NOT (target_type STREQUAL "EXECUTABLE" OR
          target_type STREQUAL "SHARED_LIBRARY" OR
          target_type STREQUAL "MODULE_LIBRARY")
  )
    return()
  endif()

  set(cest_original_extension)
  if(NOT CEST_OUTPUT_DIR)
    set(CEST_OUTPUT_DIR "tmp/")
  endif()

  if(NOT DEFINED CEST_RESOURCES_DIR)
    set(CEST_RESOURCES_DIR "resources")
  endif()

  file(REAL_PATH "${CEST_OUTPUT_DIR}" CEST_OUTPUT_DIR
       BASE_DIRECTORY "${PROJECT_BINARY_DIR}"
  )
  file(REAL_PATH "${CEST_RESOURCES_DIR}" CEST_RESOURCES_DIR
       BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  # here to create custom target and passed a custom manifest

  if(DEFINED CEST_RESOURCE_BASENAME)
    set(rcBasename ${CEST_RESOURCE_BASENAME})
  elseif(CEST_TEST)
    set(rcBasename CestTest)
    set(CestTest_icon ${rcBasename})
    set(CestTest_target ${target})
  else()
    set(rcBasename ${target})
  endif()

  if(DEFINED CEST_MANIFEST_BASENAME)
    set(cest_manifest_basename ${CEST_MANIFEST_BASENAME})
  else()
    set(cest_manifest_basename ${rcBasename})
  endif()

  set(pragma_codepage "65001")

  configure_file(
    "${CEST_RESOURCES_DIR}/${rcBasename}.rc.in"
    "${CEST_OUTPUT_DIR}/${rcBasename}_genexp.rc.in"
    @ONLY NEWLINE_STYLE LF
  )

  file(GENERATE OUTPUT "${CEST_OUTPUT_DIR}/${rcBasename}-$<CONFIG>.rc"
      INPUT "${CEST_OUTPUT_DIR}/${rcBasename}_genexp.rc.in"
      NEWLINE_STYLE UNIX
  )

  if(MINGW)
    set_source_files_properties("${CEST_OUTPUT_DIR}/${rcBasename}-Debug.rc"
      TARGET_DIRECTORY ${target}
      PROPERTIES COMPILE_DEFINITIONS $<$<CONFIG:Debug>:_DEBUG>
    )
  endif()

  target_sources(${target} PRIVATE
    "${CEST_RESOURCES_DIR}/${rcBasename}.rc.in"
    "${CEST_OUTPUT_DIR}/${rcBasename}_genexp.rc.in"
    "$<$<BOOL:$<CONFIG>>:${CEST_OUTPUT_DIR}/${rcBasename}-$<CONFIG>.rc>"
  )

  if(NOT MINGW)
    if(target_type STREQUAL "SHARED_LIBRARY" OR target_type STREQUAL "MODULE_LIBRARY")
      set(cest_original_extension "${CMAKE_SHARED_LIBRARY_SUFFIX}")
    elseif(target_type STREQUAL "EXECUTABLE")
      set(cest_original_extension "${CMAKE_EXECUTABLE_SUFFIX}")
    endif()

    target_sources(${target} PRIVATE
      "${CEST_RESOURCES_DIR}/${cest_manifest_basename}${cest_original_extension}.manifest"
    )
  endif()

endfunction()
