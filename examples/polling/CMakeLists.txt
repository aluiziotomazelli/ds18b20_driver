# components/ds18b20_driver/examples/polling/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)

list(APPEND EXTRA_COMPONENT_DIRS 
    "${PROJECT_ROOT}"
    "${PROJECT_ROOT}/external/onewire_bus/onewire_bus"
    "$ENV{IDF_PATH}/components"
)

set(COMPONENTS main ds18b20_driver)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ds18b20_polling_example)
