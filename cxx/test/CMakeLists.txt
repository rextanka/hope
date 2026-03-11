# ---------------------------------------------------------------------------
# Unit + component tests
# ---------------------------------------------------------------------------

file(GLOB_RECURSE UNIT_TEST_SOURCES CONFIGURE_DEPENDS unit/*.cpp)

add_executable(hope_tests ${UNIT_TEST_SOURCES})

target_link_libraries(hope_tests
    PRIVATE hope_lib
    PRIVATE GTest::gtest_main
    PRIVATE GTest::gmock
)

target_include_directories(hope_tests
    PRIVATE ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(hope_tests PRIVATE
    HOPE_LIB_DIR="${CMAKE_SOURCE_DIR}/../lib"
    HOPE_TEST_DIR="${CMAKE_SOURCE_DIR}/../test"
)

gtest_discover_tests(hope_tests)

# ---------------------------------------------------------------------------
# Integration tests — run the hope binary against known inputs and compare
# stdout+stderr to committed expected outputs.
# ---------------------------------------------------------------------------
add_subdirectory(integration)
