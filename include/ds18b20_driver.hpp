// components/ds18b20_driver/include/ds18b20_driver.hpp
#pragma once

#include <cstdint>

#include "interfaces/i_ds18b20_driver.hpp"
#include "interfaces/i_hal_onewire_bus.hpp"

namespace ds18b20 {

/**
 * @brief Configuration parameters for the DS18B20 driver.
 */
struct Ds18b20Config
{
    int gpio_num;                                  ///< GPIO pin where DQ line is connected
    uint32_t max_rx_bytes;                         ///< Max RX bytes buffer for RMT (e.g. 10)
    bool enable_pullup;                            ///< Enable internal pull-up if hardware resistor is not fitted
    Resolution initial_resolution{Resolution::BITS_12}; ///< Initial resolution (default 12 bits)
};

/**
 * @brief DS18B20 digital temperature sensor driver implementation.
 */
class Ds18b20Driver : public IDs18b20Driver
{
public:
    /**
     * @brief Construct the driver with injected dependencies.
     *
     * @param onewire_hal Reference to the 1-Wire bus HAL abstraction.
     * @param config Hardware pin and bus configuration.
     */
    Ds18b20Driver(IOnewireBusHAL &onewire_hal, const Ds18b20Config &config);

    /** @copydoc IDs18b20Driver::init() */
    esp_err_t init() override;

    /** @copydoc IDs18b20Driver::set_resolution() */
    esp_err_t set_resolution(Resolution resolution) override;

    /** @copydoc IDs18b20Driver::get_resolution() */
    Resolution get_resolution() const override;

    /** @copydoc IDs18b20Driver::read_temperature() */
    esp_err_t read_temperature(float *temperature) override;

    /** @copydoc IDs18b20Driver::deinit() */
    esp_err_t deinit() override;

    /**
     * @brief Calculate Dallas 1-Wire 8-bit CRC.
     *
     * Polynomial: X^8 + X^5 + X^4 + 1 (0x8C inverted / 0x31 normal).
     *
     * @param data Buffer of data bytes.
     * @param len Number of bytes.
     * @return Calculated CRC8 byte.
     */
    static uint8_t calculate_crc8(const uint8_t *data, size_t len);

    /**
     * @brief Convert raw 2-byte DS18B20 temperature to degrees Celsius.
     *
     * @param lsb Low byte of temperature register.
     * @param msb High byte of temperature register.
     * @return Temperature in degrees Celsius.
     */
    static float convert_raw_to_celsius(uint8_t lsb, uint8_t msb);

    /**
     * @brief Get conversion delay in milliseconds for a given resolution.
     *
     * @param resolution Resolution setting.
     * @return Delay in milliseconds.
     */
    static uint32_t get_conversion_delay_ms(Resolution resolution);

    /**
     * @brief DS18B20 1-Wire Family Code (0x28).
     */
    static constexpr uint8_t DS18B20_FAMILY_CODE = 0x28;

private:
    IOnewireBusHAL &onewire_hal_;
    Ds18b20Config config_;
    onewire_bus_handle_t bus_handle_{nullptr};
    onewire_device_t device_{};
    Resolution current_resolution_{Resolution::BITS_12};
    bool is_initialized_{false};
};

} // namespace ds18b20
