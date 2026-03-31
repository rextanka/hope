# Integration tests.
# Each test runs the hope binary with a .in file and diffs output against
# the corresponding .out file in cxx/test/expected/.
#
# These tests will fail until the interpreter is complete (Phase 3+).
# They are registered now so they appear in ctest output as DISABLED.

set(HOPE_BIN  $<TARGET_FILE:hope>)
set(INPUT_DIR ${CMAKE_SOURCE_DIR}/../test)
set(EXPECT_DIR ${CMAKE_SOURCE_DIR}/test/expected)
set(HOPEPATH  ${CMAKE_SOURCE_DIR}/../lib)
# Working directory for tests: src/ is a sibling of test/ so that
# relative paths like "../test/foo" inside .in files resolve correctly.
set(TEST_WORKDIR ${CMAKE_SOURCE_DIR}/../src)

set(INTEGRATION_TESTS
    hope_tut
    primes
    lambdas
    sections
    io
    type_errs
    maybe
    seq
    arith
    set
    sort
    tree
    list
    functions
    sums
    range
    ctype
    lines
    words
    lists
    products
    diag
    examples_fact
    examples_fibs
    examples_bf
    examples_boolexp
    burstall1980
    glaser1984
    bailey
    ch1_exercises
)

foreach(TEST_NAME IN LISTS INTEGRATION_TESTS)
    add_test(
        NAME "integration_${TEST_NAME}"
        COMMAND ${CMAKE_COMMAND}
            -DHOPE=${HOPE_BIN}
            -DINPUT=${INPUT_DIR}/${TEST_NAME}.in
            -DEXPECTED=${EXPECT_DIR}/${TEST_NAME}.out
            -DHOPEPATH=${HOPEPATH}
            -DWORKDIR=${TEST_WORKDIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_hope_test.cmake
    )
    # Integration tests enabled in Phase 4.
    # set_tests_properties("integration_${TEST_NAME}" PROPERTIES DISABLED TRUE)
endforeach()
