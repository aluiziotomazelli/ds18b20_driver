# DS18B20 Temperature Sensor Driver

A modern C++ driver for the Dallas DS18B20 digital temperature sensor for ESP-IDF, designed with strict Hardware Abstraction (HAL), Dependency Injection, and 100% Host-testability with GoogleTest/GMock.

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
