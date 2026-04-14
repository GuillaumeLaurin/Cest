function(cest_toolchain_requirement)
  
  set(oneValueArgs MSVC CLANG_CL GCC CLANG QT)
  cmake_parse_arguments(PARSE_ARGV 0 CEST "" "${oneValueArgs}" "")

  if(DEFINED CEST_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "The ${CMAKE_CURRENT_FUNCTION}() was passed extra arguments: \
            ${CEST_UNPARSED_ARGUMENTS}")
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS CEST_MSVC)
        message(FATAL_ERROR "Minimum required MSVC version was not satisfied, \
                required version >=${CEST_MSVC}, your version is ${CMAKE_CXX_COMPILER_VERSION}, upgrade \
                Visual Studio.")
      endif()
  endif()

  if(MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
     CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"
  )
    if(CMAKE_CXX_SIMULATE_VERSION VERSION_LESS CEST_MSVC)
      message(FATAL_ERROR "Minimum required MSVC version was not satisfied, \
              required version >=${CEST_MSVC}, your version is ${CMAKE_CXX_SIMULATE_VERSION}, upgrade \
              Visual Studio.")
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS CEST_CLANG_CL)
      message(FATAL_ERROR "Minimum required Clang-cl version was not satisfied, \
              required version >=${CEST_CLANG_CL}, your version is ${CMAKE_CXX_COMPILER_VERSION}, \
              upgrade LLVM.")
    endif()
  endif()

  if(NOT MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS CEST_GCC)
      message(STATUS "Minimum recommended GCC version was not satisfied, \
              recommended version >=${CEST_GCC}, your version is ${CMAKE_CXX_COMPILER_VERSION}, \
              upgrade the GCC compiler")
    endif()
  endif()

  if(NOT MSVC AND
    (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
     CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  )
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS CEST_CLANG)
      message(STATUS "Minimum recommended Clang version was not satisfied, \
              recommended version >=${CEST_CLANG}, your version is ${CMAKE_CXX_COMPILER_VERSION}, \
              upgrade Clang compiler")
    endif()
  endif()

endfunction()

function(cest_check_unsupported_build)

  if(MINGW AND NOT BUILD_SHARED_LIBS AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
     CMAKE_CXX_COMPILER_VERSION VERSION_LESS "18"
  )
    message(FATAL_ERROR "MinGW Clang <18 static build is not supported, it has \
            problems with inline constants :/.")
  endif()

  if(MINGW AND BUILD_SHARED_LIBS AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
     INLINE_CONSTANTS AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "18"
  )
    message(FATAL_ERROR "MinGW Clang <18 shared build crashes with inline constants, \
            don't enable the INLINE_CONSTANTS cmake option :/.")
  endif()

  if(CEST_VCPKG AND CEST_IS_MULTI_CONFIG)
    message(FATAL_ERROR "Multi-configuration generators are not supported in vcpkg \
            ports.")
  endif()

  if(CEST_VCPKG AND CEST_BUILD_LOADABLE_DRIVERS)
    message(FATAL_ERROR "Loadable SQL drivers are not supported in vcpkg ports.")
  endif()

  if(BUILD_DRIVERS AND NOT BUILD_MYSQL_DRIVER)
    message(FATAL_ERROR "If the BUILD_DRIVERS option is enabled, at least one \
            driver implementation must be enabled, please enable BUILD_MYSQL_DRIVER.")
  endif()

endfunction()
