#include <cstdio>
#include "ds18b20_driver.hpp"
#include "hal_onewire_bus.hpp"

extern "C" void app_main(void)
{
    ds18b20::OnewireBusHAL hal;
    ds18b20::Ds18b20Config config{
        .gpio_num = 4,
        .max_rx_bytes = 10,
        .enable_pullup = true,
    };

    ds18b20::Ds18b20Driver driver(hal, config);
    printf("DS18B20 target build test compiled successfully\n");
}
