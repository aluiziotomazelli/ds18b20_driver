// components/ds18b20_driver/include/interfaces/i_ds18b20_driver.hpp
#pragma once

#include <cstdint>

#include "esp_err.h"

namespace ds18b20 {

/**
 * @interface IDs18b20Driver
 * @brief Interface for the DS18B20 digital temperature sensor driver.
 */
class IDs18b20Driver
{
public:
    virtual ~IDs18b20Driver() = default;

    /**
     * @brief Initialize 1-Wire bus and find the DS18B20 sensor.
     *
     * @return ESP_OK on success.
     * @return ESP_ERR_NOT_FOUND if no DS18B20 device is detected on the bus.
     * @return ESP_ERR_INVALID_ARG if configuration is invalid.
     * @return ESP_FAIL on bus or hardware errors.
     */
    virtual esp_err_t init() = 0;

    /**
     * @brief Trigger a temperature conversion and read the result in Celsius.
     *
     * @param[out] temperature Pointer to float where temperature in Celsius will be stored.
     * @return ESP_OK on success.
     * @return ESP_ERR_INVALID_STATE if driver is not initialized.
     * @return ESP_ERR_INVALID_ARG if temperature pointer is null.
     * @return ESP_ERR_INVALID_CRC if scratchpad data CRC check fails.
     * @return ESP_FAIL on communication errors.
     */
    virtual esp_err_t read_temperature(float *temperature) = 0;

    /**
     * @brief Deinitialize and release 1-Wire bus resources.
     *
     * @return ESP_OK on success.
     */
    virtual esp_err_t deinit() = 0;
};

} // namespace ds18b20
