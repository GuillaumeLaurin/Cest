macro(cest_initialize_inline_constants_option)

  if(MINGW AND BUILD_SHARED_LIBS AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
     CMAKE_CXX_COMPILER_VERSION VERSION_LESS "18"
  )
    set(tinyInlineConstantsForceValue OFF)
  else()
    set(tinyInlineConstantsForceValue ON)
  endif()

  feature_option_dependent(INLINE_CONSTANTS
    "Use inline constants instead of extern constants in the shared build. \
    OFF is highly recommended for the shared build; is always ON for the static build"
    OFF
    "BUILD_SHARED_LIBS AND NOT ((MSVC OR MINGW) AND \
    CMAKE_CXX_COMPILER_ID STREQUAL \"Clang\" AND \
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS \"18\")"
    ${tinyInlineConstantsForceValue}
  )

  unset(tinyInlineConstantsForceValue)

endmacro()