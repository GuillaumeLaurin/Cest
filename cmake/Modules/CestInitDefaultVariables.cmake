include(CestHelpers)

macro(cest_init_cmake_variables_pre)

  if(NOT DEFINED CMAKE_EXPORT_PACKAGE_REGISTRY AND
     DEFINED ENV{CEST_EXPORT_PACKAGE_REGISTRY}
  )
    set(CMAKE_EXPORT_PACKAGE_REGISTRY "$ENV{CEST_EXPORT_PACKAGE_REGISTRY}"
        CACHE BOOL "Enables the export(PACKAGE) command, export packages \
        to the user package registry.")
  endif()

endmacro()

macro(cest_init_cmake_variables)

  set(CMAKE_DEBUG_POSTFIX d CACHE STRING
      "Default filename postfix for libraries for Debug configuration.")

  set(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL CACHE STRING
      "The default order for sorting packages found using find_package().")
  set(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC CACHE STRING
      "The sorting direction used by CMAKE_FIND_PACKAGE_SORT_ORDER.")

  set(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION ON CACHE BOOL
      "Ask cmake_install.cmake script to warn each time a file with absolute INSTALL \
      DESTINATION is encountered.")

  mark_as_advanced(
    CMAKE_DEBUG_POSTFIX
    CMAKE_FIND_PACKAGE_SORT_ORDER
    CMAKE_FIND_PACKAGE_SORT_DIRECTION
    CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION
  )

  if(MSVC AND NOT CEST_VCPKG AND NOT DEFINED VCPKG_CRT_LINKAGE AND
     DEFINED MSVC_RUNTIME_DYNAMIC AND
     NOT MSVC_RUNTIME_DYNAMIC MATCHES "(-NOTFOUND)$"
  )
    if(MSVC_RUNTIME_DYNAMIC)
      set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    else()
      set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND
     CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT
  )
    get_property(help_string CACHE CMAKE_INSTALL_PREFIX PROPERTY HELPSTRING)
    if(NOT help_string)
      set(help_string "Install path prefix, prepended onto install directories.")
    endif()

    if(MINGW)
      set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "${help_string}" FORCE)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(CMAKE_INSTALL_PREFIX "C:/Program Files/${PROJECT_NAME}"
          CACHE PATH "${help_string}" FORCE
      )
    endif()
  endif()
  set(helpStringTemplate
      "Map from <CONFIG> project configuration to an imported target's configuration.")

  string(REPLACE "<CONFIG>" "Release" release_helpString ${helpStringTemplate})
  string(REPLACE "<CONFIG>" "RelWithDebInfo" relWithDebInfo_helpString
        ${helpStringTemplate})
  string(REPLACE "<CONFIG>" "MinSizeRel" minSizeRel_helpString ${helpStringTemplate})
  string(REPLACE "<CONFIG>" "Debug" debug_helpString ${helpStringTemplate})

  set(CMAKE_MAP_IMPORTED_CONFIG_RELEASE Release RelWithDebInfo MinSizeRel ""
      CACHE STRING ${release_helpString})
  set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO RelWithDebInfo Release MinSizeRel ""
      CACHE STRING ${relWithDebInfo_helpString})
  set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL MinSizeRel RelWithDebInfo Release ""
      CACHE STRING ${minSizeRel_helpString})

  if(MSVC)
    set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Debug "" CACHE STRING ${debug_helpString})
  else()
    set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Debug RelWithDebInfo Release MinSizeRel ""
        CACHE STRING ${debug_helpString})
  endif()
  
  mark_as_advanced(
    CMAKE_MAP_IMPORTED_CONFIG_RELEASE
    CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO
    CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL
    CMAKE_MAP_IMPORTED_CONFIG_DEBUG
  )

  unset(debug_helpString)
  unset(minSizeRel_helpString)
  unset(relWithDebInfo_helpString)
  unset(release_helpString)
  unset(helpStringTemplate)

  if(VERBOSE_CONFIGURE)
    message(STATUS "${Cest_ns}: Set up defaults for \
            CMAKE_MAP_IMPORTED_CONFIG_<CONFIG> to avoid linking a release build types against debug \
            builds

            * CMAKE_MAP_IMPORTED_CONFIG_RELEASE        = ${CMAKE_MAP_IMPORTED_CONFIG_RELEASE}
            * CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO = ${CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO}
            * CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL     = ${CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL}
            * CMAKE_MAP_IMPORTED_CONFIG_DEBUG          = ${CMAKE_MAP_IMPORTED_CONFIG_DEBUG}
    ")
  endif()

  if(MINGW)
    set(CMAKE_SHARED_LIBRARY_PREFIX)
  endif()

  set(CEST_RC_FLAGS_BACKUP "")

  if(MSVC AND NOT CMAKE_RC_FLAGS MATCHES " *[-/]nologo *")
    get_property(help_string CACHE CMAKE_RC_FLAGS PROPERTY HELPSTRING)
    if(NOT help_string)
      set(help_string "Flags for Windows Resource Compiler during all build types.")
    endif()
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -nologo" CACHE STRING ${help_string} FORCE)
  endif()
  
  unset(help_string)

  cest_fix_ccache()

  if(MSVC AND CEST_VCPKG)
    string(REGEX REPLACE " *(\/|-)W[0-4] *" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  endif()

endmacro()

macro(cest_init_cest_variables_pre)

  # the main package name
  set(Cest_ns Cest)
  set(CestUtils_ns CestUtils)
  # Target names
  set(CommonConfig_target CommonConfig)
  set(Cest_target Cest)
  set(CestUtils_target CestUtils)

  string(TOLOWER "${CMAKE_BUILD_TYPE}" CEST_BUILD_TYPE_LOWER)
  string(TOUPPER "${CMAKE_BUILD_TYPE}" CEST_BUILD_TYPE_UPPER)

  get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  set(CEST_IS_MULTI_CONFIG "${isMultiConfig}" CACHE INTERNAL
      "True when using a multi-configuration generator.")
  unset(isMultiConfig)

  if(NOT CEST_VCPKG)
    set(CEST_VCPKG FALSE)
    set(CEST_PORT CEST_PORT-NOTFOUND)
  endif()

  # Auto-detect the CMAKE_TOOLCHAIN_FILE from the VCPKG_ROOT environment variable
  if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Path to the toolchain file supplied to CMake.")
  endif()
  # GitHub Actions defines the VCPKG_INSTALLATION_ROOT instead of VCPKG_ROOT
  if(DEFINED ENV{VCPKG_INSTALLATION_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE
        "$ENV{VCPKG_INSTALLATION_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Path to the toolchain file supplied to CMake.")
  endif()

  # Vcpkg CMake integration ignores VCPKG_DEFAULT_TRIPLET env. variable but accepts
  # the VCPKG_TARGET_TRIPLET command-line option
  if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING
        "Change the default triplet for CMake Integration.")
  endif()

endmacro()

macro(cest_init_cest_variables)

  define_property(GLOBAL PROPERTY CEST_PACKAGE_DEPENDENCIES
    BRIEF_DOCS "Recorded arguments from find_package() calls."
    FULL_DOCS "Recorded arguments from find_package() calls that will be used \
    to generate find_dependency() calls for the Cest package configuration file."
  )

  # Setup the correct PATH environment variable used by the ctest command
  # To debug these paths on the PATH environment variable run ctest --debug
  if(BUILD_TESTS)
    # For adjusting variables when running tests we need to know what the correct
    # variable is for separating entries in PATH-alike variables
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
      set(CEST_PATH_SEPARATOR "\\;")
    else()
      set(CEST_PATH_SEPARATOR ":")
    endif()

    # Escaped environment path
    string(REPLACE ";" "\;" CEST_TESTS_ENV "$ENV{PATH}")

    # Prepend VCPKG environment (installed folder)
    if(CEST_VCPKG)
      string(PREPEND CEST_TESTS_ENV "\
            $<SHELL_PATH:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}$<$<CONFIG:Debug>:/debug>/\
            ${CMAKE_INSTALL_BINDIR}>${CEST_PATH_SEPARATOR}\
            $<SHELL_PATH:${${Cest_ns}_BINARY_DIR}/tests/${CestUtils_ns}>${CEST_PATH_SEPARATOR}")
    else()
      if(CEST_IS_MULTI_CONFIG)
        if(CEST_BUILD_LOADABLE_DRIVERS AND BUILD_MYSQL_DRIVER)
          string(PREPEND CEST_TESTS_ENV "\
                $<SHELL_PATH:${${Cest_ns}_BINARY_DIR}/drivers/mysql/$<CONFIG>>${CEST_PATH_SEPARATOR}")
        endif()
        string(PREPEND CEST_TESTS_ENV "\
              $<SHELL_PATH:${${Cest_ns}_BINARY_DIR}/$<CONFIG>>${CEST_PATH_SEPARATOR}\
              $<SHELL_PATH:${${Cest_ns}_BINARY_DIR}/tests/${CestUtils_ns}/$<CONFIG>>${CEST_PATH_SEPARATOR}")
        endif()
      else()
        string(PREPEND CEST_TESTS_ENV "\
              $<SHELL_PATH:${${Cest_ns}_BINARY_DIR}>${CEST_PATH_SEPARATOR}\
              $<SHELL_PATH:${${Cest_ns}_BINARY_DIR}/tests/${CestUtils_ns}>${CEST_PATH_SEPARATOR}")
      endif()
    endif()

    set(CEST_BUILD_GENDIR "${Cest_ns}_generated" CACHE INTERNAL
        "Generated content in the build tree.")

    if(NOT CEST_VCPKG)
      set(CEST_VCPKG FALSE)
      set(CEST_PORT CEST_PORT-NOTFOUND)
    endif()

    if(BUILD_SHARED_LIBS AND NOT INLINE_CONSTANTS)
        set(CestExternConstants ON)
        message(VERBOSE "Using extern constants")
    else()
        set(CestExternConstants OFF)
        message(VERBOSE "Using inline constants")
    endif()
    set(CEST_EXTERN_CONSTANTS ${cestExternConstants} CACHE INTERNAL
        "Determine whether ${Cest_target} library will be built with extern or inline \
        constants.")
    unset(cestExternConstants)

endmacro()