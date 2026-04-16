add_executable(cest_tests
    main.cpp
    test_core.cpp
    test_mock.cpp
)
target_link_libraries(cest_tests PRIVATE Cest::Cest)
target_compile_features(cest_tests PRIVATE cxx_std_17)

# To test .only
add_executable(cest_test_focus test_focus.cpp)
target_link_libraries(cest_test_focus PRIVATE Cest::Cest)
target_compile_features(cest_tests PRIVATE cxx_std_17)

add_test(NAME cest_tests COMMAND cest_tests)
add_test(NAME cest_test_focus COMMAND cest_test_focus)