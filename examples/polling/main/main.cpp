/**
 * ==============================================================================
 *                     DS18B20 Hardware Wiring Diagram
 * ==============================================================================
 *
 *                  ESP32-C3
 *               +------------+
 *               |            |
 *               |       3.3V +-------------------+ (VDD - Red wire)
 *               |            |                   |
 *               |            |                  [ ] 4.7k Ohm Pull-Up Resistor
 *               |            |                   |
 *               |     GPIO 4 +-------------------+ (DQ / Data - Yellow/White wire)
 *               |            |
 *               |        GND +---------------------+ (GND - Black wire)
 *               +------------+                     |
 *                                                  |
 *                                            +-----+------+
 *                                            |  DS18B20   |
 *                                            |  Flat Face |
 *                                            |  1  2  3   |
 *                                            +--+--+--+---+
 *                                               |  |  |
 *                                         GND --+  |  +-- VDD (3.0V - 5.5V)
 *                                                  |
 *                                             DQ --+ (Data line with 4.7k pull-up)
 *
 * Pinout reference (TO-92 package with flat side facing you):
 *       Pin 1 (Left)   : GND (Ground)
 *       Pin 2 (Middle) : DQ (1-Wire Data bus)
 *       Pin 3 (Right)  : VDD (Power 3.3V)
 *
 * Typical waterproof probe color coding:
 *       RED   : VDD (3.3V)
 *       BLACK : GND (Ground)
 *       YELLOW or WHITE or BLUE : DQ (Data connected to GPIO4 with 4.7k pull-up to 3.3V)
 * ==============================================================================
 */

#include <cstdio>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"

#include "ds18b20_driver.hpp"
#include "hal_onewire_bus.hpp"

static const char* TAG = "DS18B20_EXAMPLE";

/**
 * ==============================================================================
 *                           Hardware Configuration
 * ==============================================================================
 */

/** @brief GPIO pin connected to DS18B20 DQ (Data) line */
static constexpr gpio_num_t DS18B20_GPIO = GPIO_NUM_20;

/** @brief Polling period in milliseconds */
static constexpr uint32_t POLLING_PERIOD_MS = 1000;

/**
 * @brief Application entry point.
 */
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting DS18B20 Temperature Sensor Polling Example...");

    /**
     * Step 1: Instantiate the 1-Wire Hardware Abstraction Layer (HAL).
     *
     * OnewireBusHAL wraps the ESP-IDF RMT-based 1-Wire peripheral driver.
     */
    ds18b20::OnewireBusHAL onewire_hal;

    /**
     * Step 2: Configure the DS18B20 sensor parameters.
     *
     * - gpio_num: Pin connected to 1-Wire DQ line.
     * - max_rx_bytes: Size of internal RMT buffer (10 bytes is sufficient for 9-byte scratchpad).
     * - enable_pullup: Enables ESP32 internal pull-up as a fallback (external 4.7k resistor recommended).
     * - initial_resolution: BITS_12 (0.0625 °C resolution, default).
     */
    ds18b20::Ds18b20Config config{
        .gpio_num = DS18B20_GPIO,
        .max_rx_bytes = 10,
        .enable_pullup = true,
        .initial_resolution = ds18b20::Resolution::BITS_12,
    };

    /**
     * Step 3: Instantiate the driver with Dependency Injection.
     */
    ds18b20::Ds18b20Driver sensor(onewire_hal, config);

    /**
     * Step 4: Initialize the 1-Wire bus and discover the DS18B20 device.
     */
    ESP_LOGI(TAG, "Initializing DS18B20 on GPIO %d...", DS18B20_GPIO);
    esp_err_t err = sensor.init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize DS18B20: %s", esp_err_to_name(err));
        ESP_LOGE(TAG, "Please check wiring and verify the 4.7k pull-up resistor is connected.");
        return;
    }
    ESP_LOGI(TAG, "DS18B20 sensor found and ready!");

    /**
     * Optional: Dynamically adjust measurement resolution.
     *
     * Options:
     * - ds18b20::Resolution::BITS_9  (~94 ms conversion time, 0.5 °C step)
     * - ds18b20::Resolution::BITS_10 (~188 ms conversion time, 0.25 °C step)
     * - ds18b20::Resolution::BITS_11 (~375 ms conversion time, 0.125 °C step)
     * - ds18b20::Resolution::BITS_12 (~750 ms conversion time, 0.0625 °C step)
     */
    // sensor.set_resolution(ds18b20::Resolution::BITS_12);

    /**
     * Step 5: Continuous polling loop every 1000ms.
     */
    ESP_LOGI(TAG, "Starting temperature reading loop (every %lu ms)...", (unsigned long)POLLING_PERIOD_MS);

    uint32_t sample_count = 0;
    while (true) {
        float temperature_celsius = 0.0f;

        /**
         * read_temperature() performs:
         * 1. 1-Wire Bus Reset
         * 2. Match ROM + Convert T (0x44) command
         * 3. Non-busy delay waiting for conversion to complete
         * 4. 1-Wire Bus Reset
         * 5. Match ROM + Read Scratchpad (0xBE) command
         * 6. Dallas CRC-8 checksum verification
         * 7. Conversion to degrees Celsius
         */
        err = sensor.read_temperature(&temperature_celsius);

        if (err == ESP_OK) {
            float temperature_fahrenheit = (temperature_celsius * 1.8f) + 32.0f;
            sample_count++;

            ESP_LOGI(
                TAG,
                "[#%04lu] Temperature: %.2f °C (%.2f °F)",
                (unsigned long)sample_count,
                temperature_celsius,
                temperature_fahrenheit);
        }
        else if (err == ESP_ERR_INVALID_CRC) {
            ESP_LOGW(TAG, "CRC error reading DS18B20 (possible noise on 1-Wire line)");
        }
        else {
            ESP_LOGE(TAG, "Failed to read temperature: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(POLLING_PERIOD_MS));
    }
}
