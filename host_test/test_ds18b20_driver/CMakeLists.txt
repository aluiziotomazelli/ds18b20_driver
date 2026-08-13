# components/ds18b20_driver/host_test/test_ds18b20_driver/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

include("../extra_dirs_common.cmake")

project(test_ds18b20_driver)

set(EXTRACT_PATTERNS_test_ds18b20_driver "*/ds18b20_driver/src/*.cpp" "*/ds18b20_driver/include/*.hpp")
set(REMOVE_PATTERNS_test_ds18b20_driver "*/ds18b20_driver/include/interfaces/*.hpp")

include("../coverage_common.cmake")
setup_gtest_coverage(test_ds18b20_driver test_ds18b20_driver.elf)
