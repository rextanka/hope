# run_hope_test.cmake
# Called by CTest to run one integration test.
# Parameters (set via -D on the command line):
#   HOPE      — path to the hope executable
#   INPUT     — path to the .in file
#   EXPECTED  — path to the .out file
#   HOPEPATH  — value of HOPEPATH environment variable
#   WORKDIR   — working directory (src/ sibling of test/ so ../test/ resolves)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env "HOPEPATH=${HOPEPATH}" ${HOPE} -f ${INPUT}
    WORKING_DIRECTORY ${WORKDIR}
    RESULT_VARIABLE exit_code
    OUTPUT_VARIABLE actual_out
    ERROR_VARIABLE  actual_err
)

# Merge stdout and stderr (matches the original make check behaviour)
string(APPEND actual_out ${actual_err})

file(READ ${EXPECTED} expected_out)

if(NOT actual_out STREQUAL expected_out)
    message("=== EXPECTED ===")
    message("${expected_out}")
    message("=== ACTUAL ===")
    message("${actual_out}")
    message(FATAL_ERROR "Output mismatch for ${INPUT}")
endif()
