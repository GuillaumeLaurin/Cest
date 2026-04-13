include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(CestHelpers)

set(CEST_BUILD_BUILDTREEDIR "${CEST_BUILD_GENDIR}/buildtree" CACHE INTERNAL
    "Generated content in the build tree for the build tree.")
set(CEST_BUILD_INSTALLTREEDIR "${CEST_BUILD_GENDIR}/installtree" CACHE INTERNAL
    "Generated content in the build tree for the install tree.")

function(cest_install_cest) 
  install(
    TARGETS ${Cest_target} ${CommonConfig_target}
    EXPORT CestTargets
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  if(CEST_VCPKG)
    set(cest_config_package_dir "${CMAKE_INSTALL_DATADIR}/${CEST_PORT}")
  else()
    set(cest_config_package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${Cest_ns}")
  endif()

  install(
    EXPORT CestTargets
    NAMESPACE ${Cest_ns}::
    DESTINATION "${cest_config_package_dir}"
  )

  if(CEST_VCPKG AND CEST_BUILD_TYPE_LOWER STREQUAL "debug")
    return()
  endif()

  if(NOT CEST_VCPKG)
    install(DIRECTORY "docs/" DESTINATION "${CMAKE_INSTALL_DOCDIR}/mdx")
    install(FILES AUTHORS LICENSE.md TYPE DOC)
    install(FILES NOTES.txt TYPE DOC RENAME NOTES)
    install(FILES README.md TYPE DOC RENAME README)
  else()
    cest_install_cest_vcpkg()
  endif()

  set(cest_cmake_module_path)

  install(FILES "cmake/CestPackageConfigHelpers.cmake"
          DESTINATION "${cest_config_package_dir}/Modules"
  )

  get_property(cvf_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  cest_to_bool(cvf_is_multi_config ${cvf_is_multi_config})

  cest_to_bool(cvf_is_vcpkg ${CEST_VCPKG})

  set(cest_target_includes)
  cest_generate_target_includes(cest_target_includes)

  set(BIN_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}/")
  set(CONFIG_INSTALL_DIR "${cest_config_package_dir}/")
  set(DOC_INSTALL_DIR "${CMAKE_INSTALL_DOCDIR}/")
  set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}/")
  set(LIB_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/")

  configure_package_config_file(
    "cmake/CestConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/${CEST_BUILD_INSTALLTREEDIR}/CestConfig.cmake"
    INSTALL_DESTINATION "${cest_config_package_dir}"
    PATH_VARS
    BIN_INSTALL_DIR CONFIG_INSTALL_DIR DOC_INSTALL_DIR INCLUDE_INSTALL_DIR
    LIB_INSTALL_DIR
  )

  cest_set_compatible_interface_string(${Cest_target}
    PROPERTIES VERSION_MAJOR SOVERSION
  )

  write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/${CEST_BUILD_INSTALLTREEDIR}/CestConfigVersion.cmake.in"
    COMPATIBILITY SameMajorVersion
  )

  file(READ "cmake/CestConfigVersionBuildTypeReq.cmake.in" buildTypeReqTemplate)
  file(APPEND
      "${PROJECT_BINARY_DIR}/${CEST_BUILD_INSTALLTREEDIR}/CestConfigVersion.cmake.in"
      "\n${buildTypeReqTemplate}"
  )

  configure_file(
    "${PROJECT_BINARY_DIR}/${CEST_BUILD_INSTALLTREEDIR}/CestConfigVersion.cmake.in"
    "${PROJECT_BINARY_DIR}/${CEST_BUILD_INSTALLTREEDIR}/CestConfigVersion.cmake"
    @ONLY NEWLINE_STYLE LF
  )

  install(
    FILES
    "${PROJECT_BINARY_DIR}/${CEST_BUILD_INSTALLTREEDIR}/CestConfig.cmake"
    "${PROJECT_BINARY_DIR}/${CEST_BUILD_INSTALLTREEDIR}/CestConfigVersion.cmake"
    DESTINATION "${cest_config_package_dir}"
  )

endfunction()

function(cest_install_cest_vcpkg)

  set(CestPackageVersion
    "${CEST_VERSION_MAJOR}.${CEST_VERSION_MINOR}.${CEST_VERSION_PATCH}"
  )
  configure_file("cmake/vcpkg/usage.in"
    "${CEST_BUILD_INSTALLTREEDIR}/usage"
    @ONLY NEWLINE_STYLE LF
  )

  install(FILES "${PROJECT_BINARY_DIR}/${CEST_BUILD_INSTALLTREEDIR}/usage"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${CEST_PORT}"
  )

endfunction()

# Create Package Config and Package Config Version files for the Build Tree and export it
function(cest_export_build_tree)

  set(cest_cmake_module_path)
  if(NOT CEST_VCPKG AND (MYSQL_PING OR BUILD_MYSQL_DRIVER))
    file(COPY "cmake/Modules/FindMySQL.cmake" DESTINATION "cmake/Modules")

    set(cest_cmake_module_path "\
        list(APPEND CMAKE_MODULE_PATH \"\${CMAKE_CURRENT_LIST_DIR}/cmake/Modules\")")
  endif()

  file(COPY "cmake/CestPackageConfigHelpers.cmake" DESTINATION "cmake/Modules")

  export(
    EXPORT CestTargets
    FILE "CestTargets.cmake"
    NAMESPACE ${Cest_ns}::
  )

  get_property(cvf_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  cest_to_bool(cvf_is_multi_config ${cvf_is_multi_config})

  set(cest_target_includes)
  cest_generate_target_includes(cest_target_includes)

  configure_package_config_file(
    "cmake/CestBuildTreeConfig.cmake.in"
    "CestConfig.cmake"
    INSTALL_DESTINATION "./"
    INSTALL_PREFIX "${PROJECT_BINARY_DIR}"
    NO_SET_AND_CHECK_MACRO
  )

  write_basic_package_version_file(
    "${CEST_BUILD_BUILDTREEDIR}/CestConfigVersion.cmake.in"
    COMPATIBILITY SameMajorVersion
  )

  file(READ "cmake/CestBuildTreeConfigVersionBuildTypeReq.cmake.in"
       buildTypeReqTemplate
  )
  file(APPEND
       "${PROJECT_BINARY_DIR}/${CEST_BUILD_BUILDTREEDIR}/CestConfigVersion.cmake.in"
       "\n${buildTypeReqTemplate}"
  )

  configure_file(
    "${PROJECT_BINARY_DIR}/${CEST_BUILD_BUILDTREEDIR}/CestConfigVersion.cmake.in"
    "CestConfigVersion.cmake"
    @ONLY NEWLINE_STYLE LF
  )

  export(PACKAGE ${Cest_ns})

endfunction()

function(cest_build_tree_deployment)

  if(CEST_VCPKG OR (NOT MSVC AND NOT MINGW))
    return()
  endif()

  set(filesToDeploy)

  list(LENGTH filesToDeploy filesToDeployCount)

endfunction()