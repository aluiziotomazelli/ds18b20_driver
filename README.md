# DS18B20 Driver Component

[![ESP-IDF Build](https://github.com/aluiziotomazelli/ds18b20_driver/actions/workflows/build.yml/badge.svg)](https://github.com/aluiziotomazelli/ds18b20_driver/actions/workflows/build.yml)
[![Host Tests](https://github.com/aluiziotomazelli/ds18b20_driver/actions/workflows/host_test.yml/badge.svg)](https://github.com/aluiziotomazelli/ds18b20_driver/actions/workflows/host_test.yml)
[![Coverage](https://img.shields.io/badge/coverage-report-blue)](https://aluiziotomazelli.github.io/ds18b20_driver/index.html)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A lightweight, modular, and dependency-injected C++ driver for the **Dallas DS18B20** 1-Wire digital temperature sensor, targeting **ESP-IDF v5.1+**.

## Hardware Wiring

```
                 ESP32-C3
              +------------+
              |            |
              |       3.3V +-------------------+ (VDD - Red wire)
              |            |                   |
              |            |                  [ ] 4.7k Ohm Pull-Up Resistor
              |            |                   |
              |     GPIO 4 +-------------------+ (DQ / Data - Yellow/White wire)
              |            |
              |        GND +---------------------+ (GND - Black wire)
              +------------+                     |
                                                 |
                                           +-----+------+
                                           |  DS18B20   |
                                           |  Flat Face |
                                           |  1  2  3   |
                                           +--+--+--+---+
                                              |  |  |
                                        GND --+  |  +-- VDD (3.0V - 5.5V)
                                                 |
                                            DQ --+ (Data line with 4.7k pull-up)
```

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
- **Configurable Resolution:** Supports 9, 10, 11, and 12-bit resolutions with dynamically adjusted conversion delays (100 ms to 800 ms).
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
    .initial_resolution = ds18b20::Resolution::BITS_12,
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

## Running the Polling Example

```bash
cd examples/polling
source $HOME/dev/esp/esp-idf/export.sh
idf.py set-target esp32c3
idf.py build flash monitor
```

## Running Host Tests

```bash
cd host_test/test_ds18b20_driver
source $HOME/dev/esp/esp-idf/export.sh
idf.py --preview set-target linux
idf.py build
./build/test_ds18b20_driver.elf
```

## License

This component is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

### Third-Party Acknowledgments

This component bundles and interacts with:

- **[espressif/onewire_bus](https://github.com/espressif/idf-extra-components/tree/master/onewire_bus)**: Copyright (c) Espressif Systems (Shanghai) CO LTD — Licensed under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0).

