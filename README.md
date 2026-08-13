# DS18B20 Temperature Sensor Driver

A modern C++ driver for the Dallas DS18B20 digital temperature sensor for ESP-IDF, designed with strict Hardware Abstraction (HAL), Dependency Injection, and 100% Host-testability with GoogleTest/GMock.

## Architecture

```
                       +-------------------+
                       |  IDs18b20Driver   | (Public Interface)
                       +-------------------+
                                 ^
                                 | implements
                       +-------------------+
                       |   Ds18b20Driver   |
                       +-------------------+
                                 | uses
                       +-------------------+
                       |  IOnewireBusHAL   | (HAL Interface)
                       +-------------------+
                         ^               ^
             implements /                 \ implements
    +-------------------+                 +-------------------+
    |   OnewireBusHAL   | (ESP32)         | MockOnewireBusHAL | (Host Tests)
    +-------------------+                 +-------------------+
```

## Features

- **Component-Oriented & Decoupled:** No FreeRTOS or hardware direct dependencies in business logic interface.
- **Dependency Injection:** `IOnewireBusHAL` injected through constructor.
- **CRC-8 Validation:** Scratchpad data integrity verified using Dallas CRC-8.
- **Host Testability:** Full unit test suite runnable on Linux host without ESP32 hardware.
- **Zero-heap in read loop:** Stack-based transmission buffers.

## Usage Example

```cpp
#include "ds18b20_driver.hpp"
#include "hal_onewire_bus.hpp"

// Hardware setup
ds18b20::OnewireBusHAL hal;
ds18b20::Ds18b20Config config{
    .gpio_num = 4,
    .max_rx_bytes = 10,
    .enable_pullup = true,
};

ds18b20::Ds18b20Driver sensor(hal, config);

void app_main()
{
    ESP_ERROR_CHECK(sensor.init());

    float temp_celsius = 0.0f;
    if (sensor.read_temperature(&temp_celsius) == ESP_OK) {
        printf("Temperature: %.2f C\n", temp_celsius);
    }
}
```

## Running Host Tests

```bash
cd host_test/test_ds18b20_driver
source $HOME/dev/esp/esp-idf/export.sh
idf.py --preview set-target linux
idf.py build
./build/test_ds18b20_driver.elf
```
