cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CommonModules"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules"
)

include(CestHelpers)
include(CestInitDefaultVariables)

cest_init_cest_variables_pre()

cest_read_version(CEST_VERSION
  CEST_VERSION_MAJOR CEST_VERSION_MINOR CEST_VERSION_PATCH CEST_VERSION_TWEAK
  VERSION_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/version.hpp"
  PREFIX CEST
  HEADER_FOR "${Cest_ns}"
)

project(${Cest_ns}
  DESCRIPTION "A header-only C++20 test library inspired by Jest"
  LANGUAGES CXX
  VERSION ${CEST_VERSION}
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC OFF)

set(minReqMsvcVersion    19.29)
set(minRecGCCVersion     10.2)
set(minRecClangVersion   12)
set(minReqClangClVersion 14.0.3)

include(CestToolchainRequirement)
cest_toolchain_requirement(
  MSVC      ${minReqMsvcVersion}
  CLANG_CL  ${minReqClangClVersion}
  GCC       ${minRecGCCVersion}
  CLANG     ${minRecClangVersion}
)

include(FeatureSummary)
include(CestFeatureOptions)

cest_init_cmake_variables_pre()

feature_option(BUILD_TESTS
  "Build Cest unit tests" OFF
)

feature_option_dependent(MATCH_EQUAL_EXPORTED_BUILDTREE
  "Exported package configuration from the build tree is considered to match only \
  when the build type is equal" OFF
  "CMAKE_EXPORT_PACKAGE_REGISTRY AND NOT CEST_IS_MULTI_CONFIG" OFF
)

feature_option_dependent(MSVC_RUNTIME_DYNAMIC
  "Use MSVC dynamic runtime library (-MD) instead of static (-MT), also considers \
  a Debug configuration (-MTd, -MDd)" ON
  "MSVC AND NOT CEST_VCPKG AND NOT DEFINED VCPKG_CRT_LINKAGE AND \
  NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY"
  MSVC_RUNTIME_DYNAMIC-NOTFOUND
)

feature_option_environment(STRICT_MODE
  "Propagate strict compiler/linker options and Qt definitions using \
  the ${Cest_ns}::${CommonConfig_target} interface library to the user code \
  (highly recommended; can also be set with the CEST_STRICT_MODE environment variable)"
  CEST_STRICT_MODE OFF
)

feature_option(VERBOSE_CONFIGURE
  "Show information about PACKAGES_FOUND and PACKAGES_NOT_FOUND in the configure \
  output" OFF
)

mark_as_advanced(MATCH_EQUAL_EXPORTED_BUILDTREE)

# ---
include(CestMsvcParallel)
cest_msvc_parallel("\
  Enables /MP flag for parallel builds using MSVC. Specify an \
  integer value to control the number of threads used (Only \
  works on some older versions of Visual Studio). Setting to \
  ON lets the toolchain decide how many threads to use. Set to \
  OFF to disable /MP completely."
)

cest_init_cmake_variables()
cest_init_cest_variables()

cest_check_unsupported_build()

# ---

add_library(${Cest_target} INTERFACE)
add_library(${Cest_ns}::${Cest_target} ALIAS ${Cest_target})

add_library(${Mock_target} INTERFACE)
add_library(${Cest_ns}::${Mock_target} ALIAS ${Mock_target})

add_library(${MockStrict_target} INTERFACE)
add_library(${Cest_ns}::${MockStrict_target} ALIAS ${MockStrict_target})

# ---

target_link_libraries(${Mock_target} INTERFACE ${Cest_ns}::${Cest_target})
target_link_libraries(${MockStrict_target} INTERFACE ${Cest_ns}::${Cest_target})

# ---

feature_option(CEST_EXAMPLE
  "Build the Cest example application" OFF
)

# ---

include(CestSources)
cest_sources(${Cest_target}_headers)

target_sources(${Cest_target} INTERFACE
  FILE_SET HEADERS
  BASE_DIRS ${PROJECT_SOURCE_DIR}/include
  FILES ${${Cest_target}_headers}
)

cest_sources_mock(${Mock_target}_headers)

target_sources(${Mock_target} INTERFACE
  FILE_SET HEADERS
  BASE_DIRS ${PROJECT_SOURCE_DIR}/include
  FILES ${${Mock_target}_headers}
)

target_sources(${MockStrict_target} INTERFACE
  FILE_SET HEADERS
  BASE_DIRS ${PROJECT_SOURCE_DIR}/include
  FILES ${${Mock_target}_headers}
)

# ---
include(CestCommon)
cest_mock_common(${Mock_target} ${MockStrict_target})

# ---

target_include_directories(${Mock_target}
  INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)
target_include_directories(${MockStrict_target}
  INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

# ---

set_target_properties(${Cest_target}
  PROPERTIES
    C_VISIBILITY_PRESET "hidden"
    CXX_VISIBILITY_PRESET "hidden"
    VISIBILITY_INLINES_HIDDEN YES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    EXPORT_NAME ${Cest_ns}
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set_property(
    TARGET ${Cest_target}
    PROPERTY OUTPUT_NAME "${Cest_target}${PROJECT_VERSION_MAJOR}"
  )
endif()

target_include_directories(${Cest_target}
  INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

# ---

target_compile_definitions(${Cest_target}
  INTERFACE
    PROJECT_CEST
    # Release build
    $<$<NOT:$<CONFIG:Debug>>:CEST_NO_DEBUG>
    # Debug build
    $<$<CONFIG:Debug>:CEST_DEBUG>
)

# ---

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  cest_set_rc_flags("-I \"${PROJECT_SOURCE_DIR}/resources\"")
endif()

include(CestResourceAndManifest)
cest_resource_and_manifest(${Cest_target}
  OUTPUT_DIR "${CEST_BUILD_GENDIR}/tmp/"
)

# ---

include(CestCommon)
cest_common(${CommonConfig_target}
  NAMESPACE ${Cest_ns}
  EXPORT NAME ${CommonConfig_target}
)

if(STRICT_MODE)
  target_link_libraries(${Cest_target}
    PUBLIC ${Cest_ns}::${CommonConfig_target}
  )
else()
  target_link_libraries(${Cest_target}
    INTERFACE ${Cest_ns}::${CommonConfig_target}
  )
endif()

if(BUILD_TESTS)
  enable_testing()

  add_subdirectory(tests)
endif()

if(CEST_EXAMPLE)
  add_subdirectory(examples)
endif()

# ---

include(CestDeployment)

cest_generate_find_dependency_calls(cest_find_dependency_calls)

cest_install_cest()

cest_export_build_tree()

if(BUILD_TREE_DEPLOY)
  cest_build_tree_deployment()
endif()

# ---
include(FeatureSummary)

#set_packages_properties()

if(VERBOSE_CONFIGURE)
  if(NOT CEST_IS_MULTI_CONFIG AND NOT CEST_BUILD_TYPE_LOWER STREQUAL "debug")
    message(STATUS "Disabled debug output and asserts")
  endif()

  feature_summary(WHAT ALL)
else()
  feature_summary(WHAT ENABLED_FEATURES DISABLED_FEATURES)
endif()