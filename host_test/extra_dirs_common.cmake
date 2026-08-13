# components/ds18b20_driver/host_test/extra_dirs_common.cmake
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

list(APPEND EXTRA_COMPONENT_DIRS
    "${PROJECT_ROOT}"
    "${PROJECT_ROOT}/host_test/gtest"
)

set(COMPONENTS 
    "main"
    "ds18b20_driver"
    "gtest"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
