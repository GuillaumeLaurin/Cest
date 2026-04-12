function(cest_sources out_headers)

  # Header files section
  set(headers)

  list(APPEND headers
    cest/core.hpp
    cest/hot_patch.hpp
    cest/mock.hpp
    cest.hpp
    version.hpp
  )

  list(SORT headers)
  list(SORT sources)

  list(TRANSFORM headers PREPEND "include/cest/")

  set(${out_headers} ${headers} PARENT_SCOPE)

endfunction()