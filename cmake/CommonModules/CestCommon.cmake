function(cest_common target)
  set(options EXPORT)
  set(oneValueArgs NAMESPACE NAME)
  cmake_parse_arguments(PARSE_ARGV 1 CEST "${options}" "${oneValueArgs}" "")

  if(DEFINED CEST_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "The ${CMAKE_CURRENT_FUNCTION}() was passed extra arguments: \
            ${CEST_UNPARSED_ARGUMENTS}")
  endif()

  add_library(${target} INTERFACE)
  add_library(${CEST_NAMESPACE}::${CEST_NAME} ALIAS ${target})

  if(CEST_EXPORT)
    set_target_properties(${target} PROPERTIES EXPORT_NAME ${CEST_NAMESPACE})
  endif()

  target_compile_features(${target} INTERFACE cxx_std_20)

  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(${target} INTERFACE
      # Windows 11 "22H2" - 0x0A00000C
      NTDDI_VERSION=NTDDI_WIN10_NI
      # Internet Explorer 11
      _WIN32_IE=_WIN32_IE_IE110
      # Exclude unneeded header files
      WIN32_LEAN_AND_MEAN
      NOMINMAX
      # Others
      UNICODE _UNICODE
    )
  endif()

  if(MSVC)
    target_compile_options(${target} INTERFACE
      /nologo
      /EHsc
      /utf-8
      /Zc:__cplusplus
      /Zc:strictStrings
      # Improves conformance
      /permissive-
      /std:c++20
    )

    if(NOT CEST_VCPKG)
      target_compile_options(${target} INTERFACE
        /W4
        $<$<CONFIG:Debug>:/WX /sdl>
      )
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      target_compile_options(${target} INTERFACE
        /permissive-
        /guard:cf
        /bigobj
        # Standards-conforming behavior
        /Zc:wchar_t,rvalueCast,inline
        /Zc:throwingNew,referenceBinding,ternary
        # C/C++ conformant preprocessor
        /Zc:preprocessor
        /external:anglebrackets /external:W0
        # Enable and check it from time to time
        # /external:templates-
        /wd4702
      )

      if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "19.38.32914.95")
        target_compile_definitions(${target} INTERFACE
          _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
        )
      endif()
    endif()

    target_link_options(${target} INTERFACE
      /guard:cf
      $<$<NOT:$<CONFIG:Debug>>:/OPT:REF,ICF=5>
      # /OPT:REF,ICF does not support incremental linking
      $<$<CONFIG:RelWithDebInfo>:/INCREMENTAL:NO>
      # Abort linking on warnings for Debug builds only, Release builds must go on
      # as far as possible
      $<$<CONFIG:Debug>:/WX>
    )
  endif()

  if(MINGW)
    target_compile_options(${target} INTERFACE
      $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-ignored-attributes>
    )
  endif()

  if(NOT MSVC AND
    (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
     CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
     CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  )
    if(NOT CEST_VCPKG)
      target_compile_options(${target} INTERFACE
        $<$<CONFIG:Debug>:-Werror -Wfatal-errors -pedantic-errors>
      )
    endif()

    target_compile_options(${target} INTERFACE
      -Wall
      -Wextra
      -pedantic
      -Wcast-qual
      -Wcast-align
      -Woverloaded-virtual
      -Wold-style-cast
      -Wshadow
      -Wundef
      -Wfloat-equal
      -Wformat-security
      -Wdouble-promotion
      -Wconversion
      -Wzero-as-null-pointer-constant
      -Wuninitialized
      -Wdeprecated-copy-dtor
      # Reduce I/O operations (use pipes between commands when possible)
      -pipe
    )

    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(-Wstrict-null-sentinel SNS_SUPPORT)

    if(SNS_SUPPORT)
      target_compile_options(${target} INTERFACE -Wstrict-null-sentinel)
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      target_compile_options(${target} INTERFACE -Wdeprecated)
    endif()
  endif()

  if(NOT MINGW AND NOT MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_options(${target} INTERFACE -fuse-ld=lld)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    target_compile_definitions(${target} INTERFACE -D_FILE_OFFSET_BITS=64)
  endif()

endfunction()

function(cest_mock_common mock_target mock_strict_target)

  if(MSVC)
    foreach(facet IN ITEMS ${mock_target} ${mock_strict_target})
      target_compile_options(${facet} INTERFACE /guard:cf-)
      target_link_options(${facet}    INTERFACE /GUARD:NO /INCREMENTAL:NO)
    endforeach()

    target_compile_options(${mock_target} INTERFACE /Ob0)
  else()
    target_compile_definitions(${mock_target} INTERFACE
      -fno-inline -fno-inline-functions
    )
  endif()
  
endfunction()
