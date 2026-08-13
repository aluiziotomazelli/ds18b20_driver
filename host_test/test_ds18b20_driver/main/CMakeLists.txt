idf_component_register(
    SRCS 
        "main.cpp"
        "test_ds18b20_driver.cpp"
    INCLUDE_DIRS
        "."
        "../../../mocks"
    REQUIRES 
        ds18b20_driver
        gtest
        WHOLE_ARCHIVE
)

if(IDF_TARGET STREQUAL "linux")
    target_compile_options(${COMPONENT_LIB} PUBLIC --coverage)
    target_link_options(${COMPONENT_LIB} PUBLIC --coverage)
endif()
