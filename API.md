# DS18B20 Driver Component API Reference

Detailed programming interface reference for the `ds18b20_driver` component.

---

## Data Types

### `Resolution`
```cpp
enum class Resolution : uint8_t {
    BITS_9  = 0x1F,  ///< 9-bit resolution (0.5 °C, max 93.75 ms conversion)
    BITS_10 = 0x3F,  ///< 10-bit resolution (0.25 °C, max 187.5 ms conversion)
    BITS_11 = 0x5F,  ///< 11-bit resolution (0.125 °C, max 375 ms conversion)
    BITS_12 = 0x7F,  ///< 12-bit resolution (0.0625 °C, max 750 ms conversion - default)
};
```
DS18B20 measurement resolution settings mapped to the configuration register byte (Byte 4 of Scratchpad).

### `Ds18b20Config`
```cpp
struct Ds18b20Config {
    int gpio_num;                                       ///< GPIO pin where DQ line is connected.
    uint32_t max_rx_bytes;                              ///< Max RX bytes buffer for RMT (e.g. 10).
    bool enable_pullup;                                 ///< Enable internal pull-up if hardware resistor is not fitted.
    Resolution initial_resolution{Resolution::BITS_12}; ///< Initial resolution applied during init() (default 12-bit).
};
```
Configuration parameters used to instantiate and initialize the DS18B20 sensor.

### `DS18B20_FAMILY_CODE`
```cpp
static constexpr uint8_t DS18B20_FAMILY_CODE = 0x28;
```
Expected 1-Wire Family Code (least significant byte of the 64-bit ROM ID), identifying the device as a DS18B20 digital thermometer.

---

## Interfaces

### `IDs18b20Driver` (Abstract Driver Interface)
Defined in `include/interfaces/i_ds18b20_driver.hpp`.

#### `virtual esp_err_t init() = 0`
Initializes the 1-Wire bus peripheral and searches for the first DS18B20 device on the bus. If `initial_resolution` is configured to a non-default resolution (other than 12-bit), applies it during initialization.
- **Returns**:
  - `ESP_OK`: Successfully initialized and device found.
  - `ESP_ERR_NOT_FOUND`: 1-Wire bus reset succeeded but no device with Family Code `0x28` was detected.
  - `ESP_ERR_INVALID_ARG`: Invalid bus or hardware configuration.
  - `ESP_FAIL`: Communication failure.

#### `virtual esp_err_t set_resolution(Resolution resolution) = 0`
Sets the temperature measurement resolution by writing to the DS18B20 configuration register via the `Write Scratchpad (0x4E)` command.
- **Parameters**: `resolution` - Desired resolution (`BITS_9`, `BITS_10`, `BITS_11`, or `BITS_12`).
- **Returns**:
  - `ESP_OK`: Resolution successfully written to sensor scratchpad.
  - `ESP_ERR_INVALID_STATE`: Driver is not initialized.
  - `ESP_FAIL`: Bus communication failure.

#### `virtual Resolution get_resolution() const = 0`
Returns the active measurement resolution configured on the driver.
- **Returns**: Current `Resolution` setting.

#### `virtual esp_err_t read_temperature(float *temperature) = 0`
Triggers a temperature conversion (`Convert T (0x44)`), waits for the conversion time corresponding to the active resolution, reads the 9-byte scratchpad (`Read Scratchpad (0xBE)`), verifies data integrity via Dallas CRC-8, and converts the raw 16-bit register value to Celsius.
- **Parameters**: `temperature` - Pointer to float where the calculated temperature in degrees Celsius will be stored.
- **Returns**:
  - `ESP_OK`: Successful measurement and CRC verification.
  - `ESP_ERR_INVALID_ARG`: `temperature` pointer is `nullptr`.
  - `ESP_ERR_INVALID_STATE`: Driver is not initialized.
  - `ESP_ERR_INVALID_CRC`: Scratchpad CRC checksum verification failed (noise on 1-Wire bus).
  - `ESP_FAIL`: Bus reset or communication error.

#### `virtual esp_err_t deinit() = 0`
Releases the 1-Wire bus handle and resets internal driver state.
- **Returns**: `ESP_OK` on success.

---

### `IOnewireBusHAL` (1-Wire Hardware Abstraction Layer Interface)
Defined in `include/interfaces/i_hal_onewire_bus.hpp`.

Abstracts the low-level Espressif 1-Wire bus driver for testability and dependency injection.

- `virtual esp_err_t new_bus_rmt(const onewire_bus_config_t *bus_config, const onewire_bus_rmt_config_t *rmt_config, onewire_bus_handle_t *ret_bus) = 0`
- `virtual esp_err_t bus_del(onewire_bus_handle_t bus) = 0`
- `virtual esp_err_t bus_reset(onewire_bus_handle_t bus) = 0`
- `virtual esp_err_t write_bytes(onewire_bus_handle_t bus, const uint8_t *tx_data, uint8_t tx_data_size) = 0`
- `virtual esp_err_t read_bytes(onewire_bus_handle_t bus, uint8_t *rx_buf, size_t rx_buf_size) = 0`
- `virtual esp_err_t write_bit(onewire_bus_handle_t bus, uint8_t tx_bit) = 0`
- `virtual esp_err_t read_bit(onewire_bus_handle_t bus, uint8_t *rx_bit) = 0`
- `virtual esp_err_t new_device_iter(onewire_bus_handle_t bus, onewire_device_iter_handle_t *ret_iter) = 0`
- `virtual esp_err_t del_device_iter(onewire_device_iter_handle_t iter) = 0`
- `virtual esp_err_t device_iter_get_next(onewire_device_iter_handle_t iter, onewire_device_t *dev) = 0`

---

## Classes

### `Ds18b20Driver` (Concrete Driver Implementation)
Defined in `include/ds18b20_driver.hpp`. Implements `IDs18b20Driver` over `IOnewireBusHAL`.

#### `Ds18b20Driver(IOnewireBusHAL &onewire_hal, const Ds18b20Config &config)`
Constructs the driver with injected HAL and hardware configuration.
- **Parameters**:
  - `onewire_hal`: Reference to `IOnewireBusHAL` (enables mock injection in unit tests).
  - `config`: Pin, buffer, and initial resolution parameters.

#### `static uint8_t calculate_crc8(const uint8_t *data, size_t len)`
Calculates the Dallas 1-Wire 8-bit cyclic redundancy check ($X^8 + X^5 + X^4 + 1$).
- **Parameters**:
  - `data`: Pointer to byte buffer.
  - `len`: Length of buffer in bytes.
- **Returns**: Calculated 8-bit CRC value.

#### `static float convert_raw_to_celsius(uint8_t lsb, uint8_t msb)`
Converts raw 2-byte 12-bit two's complement temperature data from the DS18B20 scratchpad into floating-point Celsius.
- **Parameters**:
  - `lsb`: Low byte of temperature register (Byte 0).
  - `msb`: High byte of temperature register (Byte 1).
- **Returns**: Temperature in degrees Celsius ($LSB \times 0.0625^\circ\text{C}$).

#### `static uint32_t get_conversion_delay_ms(Resolution resolution)`
Returns the recommended conversion wait time in milliseconds for a given resolution:
- `BITS_9`: 100 ms
- `BITS_10`: 200 ms
- `BITS_11`: 400 ms
- `BITS_12`: 800 ms

---

### `OnewireBusHAL` (Concrete 1-Wire HAL Implementation)
Defined in `include/hal_onewire_bus.hpp`.

Thin, pass-through wrapper over the ESP-IDF `onewire_bus` driver (RMT backend). Excluded from host builds (Linux) where `MockOnewireBusHAL` is used instead.
