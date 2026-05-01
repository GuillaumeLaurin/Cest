function(cest_sources out_headers)

  # Header files section
  set(headers)

  list(APPEND headers
    cest/macros/stringify.hpp
    cest/macros/compiler_detect.hpp
    cest/core.hpp
    cest/math.hpp
    cest.hpp
    version.hpp
  )

  list(SORT headers)
  list(SORT sources)

  list(TRANSFORM headers PREPEND "include/")

  set(${out_headers} ${headers} PARENT_SCOPE)

endfunction()

function(cest_sources_mock out_headers)

  # Header files section
  set(headers)

  list(APPEND headers
    cest/macros/attributes.hpp
    cest/mocks/mock.hpp
    cest/mocks/hot_patch.hpp
    mocks.hpp
  )

  list(SORT headers)
  list(SORT sources)

  list(TRANSFORM headers PREPEND "include/")

  set(${out_headers} ${headers} PARENT_SCOPE)

endfunction()