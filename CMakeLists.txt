set(COMPONENT_NAME ds18b20_driver)

if(IDF_TARGET STREQUAL "linux")
    idf_component_register(
        SRCS
            "src/ds18b20_driver.cpp"
        INCLUDE_DIRS
            "include"
            "include/interfaces"
        REQUIRES
            esp_timer
    )

    target_compile_options(${COMPONENT_LIB} PUBLIC --coverage)
    target_link_options(${COMPONENT_LIB} PUBLIC --coverage)
else()
    idf_component_register(
        SRCS
            "src/hal_onewire_bus.cpp"
            "src/ds18b20_driver.cpp"
        INCLUDE_DIRS
            "include"
            "include/interfaces"
        REQUIRES
            onewire_bus
            esp_timer
            freertos
    )
endif()
