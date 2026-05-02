# tests/CMakeLists.txt

add_executable(cest_tests
    main.cpp
    test_core.cpp
    test_custom.cpp
)
target_link_libraries(cest_tests PRIVATE Cest::Cest)

add_executable(cest_focus_tests test_focus.cpp)
target_link_libraries(cest_focus_tests PRIVATE Cest::Cest)

add_executable(cest_mock_tests test_mock.cpp)
target_link_libraries(cest_mock_tests PRIVATE Cest::Mock)

# Hot-patch tests are only built and run where hot-patching actually works.
# - macOS: blocked by W^X enforcement on the __TEXT segment.
# - Release builds: optimizer can bypass patches (constant propagation,
#   IPA cloning) even with CEST_MOCKABLE on MSVC (no equivalent of GCC's
#   noipa available).
if(NOT APPLE)
  add_executable(cest_hotpatch_tests test_hotpatch.cpp)
  target_link_libraries(cest_hotpatch_tests PRIVATE Cest::Mock)

  add_executable(cest_strict_hotpatch_tests test_strict_hotpatch.cpp)
  target_link_libraries(cest_strict_hotpatch_tests PRIVATE Cest::MockStrict)

  add_test(NAME cest_hotpatch_tests
           COMMAND cest_hotpatch_tests
           CONFIGURATIONS Debug)
  add_test(NAME cest_strict_hotpatch_tests
           COMMAND cest_strict_hotpatch_tests
           CONFIGURATIONS Debug)
endif()

add_test(NAME cest_tests        COMMAND cest_tests)
add_test(NAME cest_focus_tests  COMMAND cest_focus_tests)
add_test(NAME cest_mock_tests   COMMAND cest_mock_tests)