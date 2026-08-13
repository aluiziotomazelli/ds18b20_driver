# components/ds18b20_driver/host_test/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(ds18b20_driver_host_tests NONE)

include(CTest)

add_test(NAME test_ds18b20_driver COMMAND ../test_ds18b20_driver/build/test_ds18b20_driver.elf)

find_program(LCOV_PATH lcov REQUIRED)
find_program(GENHTML_PATH genhtml REQUIRED)

set(UNIFIED_COVERAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/coverage")

add_custom_target(unified_coverage
    COMMAND ${LCOV_PATH} --capture --directory .. --output-file unified_coverage.info --rc branch_coverage=1 --rc geninfo_unexecuted_blocks=1 --ignore-errors mismatch,inconsistent
    COMMAND ${LCOV_PATH} --extract unified_coverage.info "*/ds18b20_driver/src/*" --output-file unified_coverage_filtered.info --rc branch_coverage=1 --ignore-errors mismatch,inconsistent,unused,empty
    COMMAND ${LCOV_PATH} --remove unified_coverage_filtered.info "*/ds18b20_driver/include/interfaces/*" --output-file unified_coverage_filtered.info --rc branch_coverage=1 --ignore-errors mismatch,inconsistent,unused,empty
    COMMAND ${GENHTML_PATH} unified_coverage_filtered.info --output-directory ${UNIFIED_COVERAGE_DIR} --title "DS18B20 Driver Coverage" --rc branch_coverage=1 --ignore-errors mismatch,inconsistent,unused
    COMMAND ${LCOV_PATH} --list unified_coverage_filtered.info --rc branch_coverage=1 --ignore-errors mismatch,inconsistent,unused,empty
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating unified coverage report from all host tests"
)

add_custom_target(coverage_clean
    COMMAND find . -name "*.gcda" -delete
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${UNIFIED_COVERAGE_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Cleaning up coverage data"
)

add_custom_target(build_all_tests
    COMMAND idf.py -C ../test_ds18b20_driver build
    COMMENT "Building all test projects using idf.py"
)

add_custom_target(run_all_tests
    COMMAND ${CMAKE_COMMAND} --build . --target build_all_tests
    COMMAND ${CMAKE_COMMAND} --build . --target coverage_clean
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND ${CMAKE_COMMAND} --build . --target unified_coverage
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Full test cycle: build, clean, run, and coverage"
)
