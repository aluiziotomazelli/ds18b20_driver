// components/ds18b20_driver/src/ds18b20_driver.cpp
#include "ds18b20_driver.hpp"

#include <cstring>

#if !defined(CONFIG_IDF_TARGET_LINUX)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"

static const char *TAG = "Ds18b20Driver";

namespace ds18b20 {

namespace {
constexpr uint8_t CMD_MATCH_ROM       = 0x55;
constexpr uint8_t CMD_CONVERT_T       = 0x44;
constexpr uint8_t CMD_READ_SCRATCHPAD = 0xBE;
constexpr size_t  SCRATCHPAD_SIZE     = 9;
constexpr size_t  ROM_ID_SIZE         = 8;
} // namespace

Ds18b20Driver::Ds18b20Driver(IOnewireBusHAL &onewire_hal, const Ds18b20Config &config)
    : onewire_hal_(onewire_hal)
    , config_(config)
{
}

esp_err_t Ds18b20Driver::init()
{
    if (is_initialized_) {
        return ESP_OK;
    }

    if (config_.max_rx_bytes == 0) {
        config_.max_rx_bytes = 10;
    }

    onewire_bus_config_t bus_config = {};
    bus_config.bus_gpio_num = config_.gpio_num;
    bus_config.flags.en_pull_up = config_.enable_pullup ? 1U : 0U;

    onewire_bus_rmt_config_t rmt_config = {};
    rmt_config.max_rx_bytes = config_.max_rx_bytes;

    esp_err_t err = onewire_hal_.new_bus_rmt(&bus_config, &rmt_config, &bus_handle_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create 1-Wire bus: %s", esp_err_to_name(err));
        return err;
    }

    onewire_device_iter_handle_t iter = nullptr;
    err = onewire_hal_.new_device_iter(bus_handle_, &iter);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create device iterator: %s", esp_err_to_name(err));
        onewire_hal_.bus_del(bus_handle_);
        bus_handle_ = nullptr;
        return err;
    }

    bool found = false;
    onewire_device_t dev = {};
    while (onewire_hal_.device_iter_get_next(iter, &dev) == ESP_OK) {
        uint8_t family_code = static_cast<uint8_t>(dev.address & 0xFF);
        if (family_code == DS18B20_FAMILY_CODE) {
            device_ = dev;
            found = true;
            break;
        }
    }

    onewire_hal_.del_device_iter(iter);

    if (!found) {
        ESP_LOGW(TAG, "No DS18B20 device found on bus");
        onewire_hal_.bus_del(bus_handle_);
        bus_handle_ = nullptr;
        return ESP_ERR_NOT_FOUND;
    }

    is_initialized_ = true;
    ESP_LOGI(TAG, "DS18B20 initialized successfully, ROM: 0x%016llX", (unsigned long long)device_.address);
    return ESP_OK;
}

esp_err_t Ds18b20Driver::read_temperature(float *temperature)
{
    if (temperature == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!is_initialized_ || bus_handle_ == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    // Step 1: Bus reset
    esp_err_t err = onewire_hal_.bus_reset(bus_handle_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Bus reset failed before convert: %s", esp_err_to_name(err));
        return err;
    }

    // Step 2: Match ROM + Convert T
    uint8_t convert_cmd[1 + ROM_ID_SIZE + 1];
    convert_cmd[0] = CMD_MATCH_ROM;
    std::memcpy(&convert_cmd[1], &device_.address, ROM_ID_SIZE);
    convert_cmd[1 + ROM_ID_SIZE] = CMD_CONVERT_T;

    err = onewire_hal_.write_bytes(bus_handle_, convert_cmd, sizeof(convert_cmd));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send Convert T command: %s", esp_err_to_name(err));
        return err;
    }

#if !defined(CONFIG_IDF_TARGET_LINUX)
    // Wait for conversion (up to 750ms for 12-bit resolution)
    vTaskDelay(pdMS_TO_TICKS(800));
#endif

    // Step 3: Bus reset
    err = onewire_hal_.bus_reset(bus_handle_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Bus reset failed before read: %s", esp_err_to_name(err));
        return err;
    }

    // Step 4: Match ROM + Read Scratchpad
    uint8_t read_cmd[1 + ROM_ID_SIZE + 1];
    read_cmd[0] = CMD_MATCH_ROM;
    std::memcpy(&read_cmd[1], &device_.address, ROM_ID_SIZE);
    read_cmd[1 + ROM_ID_SIZE] = CMD_READ_SCRATCHPAD;

    err = onewire_hal_.write_bytes(bus_handle_, read_cmd, sizeof(read_cmd));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send Read Scratchpad command: %s", esp_err_to_name(err));
        return err;
    }

    // Step 5: Read 9 bytes of scratchpad
    uint8_t scratchpad[SCRATCHPAD_SIZE] = {0};
    err = onewire_hal_.read_bytes(bus_handle_, scratchpad, SCRATCHPAD_SIZE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read scratchpad bytes: %s", esp_err_to_name(err));
        return err;
    }

    // Step 6: Validate CRC8
    uint8_t calculated_crc = calculate_crc8(scratchpad, SCRATCHPAD_SIZE - 1);
    if (calculated_crc != scratchpad[SCRATCHPAD_SIZE - 1]) {
        ESP_LOGE(TAG, "CRC mismatch: calculated 0x%02X, received 0x%02X", calculated_crc, scratchpad[SCRATCHPAD_SIZE - 1]);
        return ESP_ERR_INVALID_CRC;
    }

    // Step 7: Convert temperature
    *temperature = convert_raw_to_celsius(scratchpad[0], scratchpad[1]);
    return ESP_OK;
}

esp_err_t Ds18b20Driver::deinit()
{
    if (bus_handle_ != nullptr) {
        onewire_hal_.bus_del(bus_handle_);
        bus_handle_ = nullptr;
    }
    is_initialized_ = false;
    device_ = {};
    return ESP_OK;
}

uint8_t Ds18b20Driver::calculate_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < len; ++i) {
        uint8_t inbyte = data[i];
        for (uint8_t j = 0; j < 8; ++j) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8C;
            }
            inbyte >>= 1;
        }
    }
    return crc;
}

float Ds18b20Driver::convert_raw_to_celsius(uint8_t lsb, uint8_t msb)
{
    int16_t raw = static_cast<int16_t>((static_cast<uint16_t>(msb) << 8) | lsb);
    return static_cast<float>(raw) * 0.0625f;
}

} // namespace ds18b20
