// components/ds18b20_driver/mocks/mock_ds18b20_driver.hpp
#pragma once

#include <gmock/gmock.h>

#include "interfaces/i_ds18b20_driver.hpp"

namespace ds18b20 {

class MockDs18b20Driver : public IDs18b20Driver
{
public:
    MOCK_METHOD(esp_err_t, init, (), (override));
    MOCK_METHOD(esp_err_t, read_temperature, (float *temperature), (override));
    MOCK_METHOD(esp_err_t, deinit, (), (override));
};

} // namespace ds18b20
