add_executable(cest_tests
    main.cpp
    test_core.cpp
    test_mock.cpp
)
target_link_libraries(cest_tests PRIVATE Cest::Cest)
target_compile_features(cest_tests PRIVATE cxx_std_17)

add_test(NAME cest_tests COMMAND cest_tests)