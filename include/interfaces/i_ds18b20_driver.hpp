// components/ds18b20_driver/include/interfaces/i_ds18b20_driver.hpp
#pragma once

#include <cstdint>

#include "esp_err.h"

namespace ds18b20 {

/**
 * @brief DS18B20 temperature measurement resolution.
 */
enum class Resolution : uint8_t
{
    BITS_9  = 0x1F,  ///< 9-bit resolution (0.5 °C, max 93.75 ms conversion)
    BITS_10 = 0x3F,  ///< 10-bit resolution (0.25 °C, max 187.5 ms conversion)
    BITS_11 = 0x5F,  ///< 11-bit resolution (0.125 °C, max 375 ms conversion)
    BITS_12 = 0x7F,  ///< 12-bit resolution (0.0625 °C, max 750 ms conversion - default)
};

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
     * @brief Set the temperature measurement resolution.
     *
     * Writes to the sensor's scratchpad configuration register (0x4E).
     *
     * @param resolution Desired resolution (9, 10, 11, or 12 bits).
     * @return ESP_OK on success.
     * @return ESP_ERR_INVALID_STATE if driver is not initialized.
     * @return ESP_FAIL on communication error.
     */
    virtual esp_err_t set_resolution(Resolution resolution) = 0;

    /**
     * @brief Get the currently configured resolution.
     *
     * @return Current resolution setting.
     */
    virtual Resolution get_resolution() const = 0;

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
