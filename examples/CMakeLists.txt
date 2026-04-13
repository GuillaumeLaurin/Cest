add_executable(cest_examples
    main.cpp
    01_basic_matcher.cpp
    02_mock_functions_basics.cpp
)
target_link_libraries(cest_examples PRIVATE Cest::Cest)
target_compile_features(cest_examples PRIVATE cxx_std_17)