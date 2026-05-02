add_executable(cest_tests
    main.cpp
    test_core.cpp
    test_custom.cpp
)
target_link_libraries(cest_tests PRIVATE Cest::Cest)

# To test .only
add_executable(cest_focus_tests test_focus.cpp)
target_link_libraries(cest_focus_tests PRIVATE Cest::Cest)

# To test mock (Cest::Mock — no annotation required, inlining disabled)
add_executable(cest_mock_tests test_mock.cpp)
target_link_libraries(cest_mock_tests PRIVATE Cest::Mock)

# To test hotpatch (low-level API, also via Cest::Mock)
add_executable(cest_hotpatch_tests test_hotpatch.cpp)
target_link_libraries(cest_hotpatch_tests PRIVATE Cest::Mock)

# To test strict hotpatch (Cest::MockStrict — inlining kept, CEST_MOCKABLE required)
add_executable(cest_strict_hotpatch_tests test_strict_hotpatch.cpp)
target_link_libraries(cest_strict_hotpatch_tests PRIVATE Cest::MockStrict)

add_test(NAME cest_tests               COMMAND cest_tests)
add_test(NAME cest_focus_tests         COMMAND cest_focus_tests)
add_test(NAME cest_mock_tests          COMMAND cest_mock_tests)

add_test(NAME cest_hotpatch_tests      
         COMMAND cest_hotpatch_tests
         CONFIGURATION Debug)
         
add_test(NAME cest_strict_hotpatch_tests 
         COMMAND cest_strict_hotpatch_tests
         CONFIGURATION Debug)