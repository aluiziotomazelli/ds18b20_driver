// components/ds18b20_driver/mocks/mock_hal_onewire_bus.hpp
#pragma once

#include <gmock/gmock.h>

#include "interfaces/i_hal_onewire_bus.hpp"

namespace ds18b20 {

class MockOnewireBusHAL : public IOnewireBusHAL
{
public:
    MOCK_METHOD(esp_err_t, new_bus_rmt,
                (const onewire_bus_config_t *bus_config,
                 const onewire_bus_rmt_config_t *rmt_config,
                 onewire_bus_handle_t *ret_bus),
                (override));

    MOCK_METHOD(esp_err_t, bus_del, (onewire_bus_handle_t bus), (override));

    MOCK_METHOD(esp_err_t, bus_reset, (onewire_bus_handle_t bus), (override));

    MOCK_METHOD(esp_err_t, write_bytes,
                (onewire_bus_handle_t bus, const uint8_t *tx_data, uint8_t tx_data_size),
                (override));

    MOCK_METHOD(esp_err_t, read_bytes,
                (onewire_bus_handle_t bus, uint8_t *rx_buf, size_t rx_buf_size),
                (override));

    MOCK_METHOD(esp_err_t, write_bit,
                (onewire_bus_handle_t bus, uint8_t tx_bit),
                (override));

    MOCK_METHOD(esp_err_t, read_bit,
                (onewire_bus_handle_t bus, uint8_t *rx_bit),
                (override));

    MOCK_METHOD(esp_err_t, new_device_iter,
                (onewire_bus_handle_t bus, onewire_device_iter_handle_t *ret_iter),
                (override));

    MOCK_METHOD(esp_err_t, del_device_iter,
                (onewire_device_iter_handle_t iter),
                (override));

    MOCK_METHOD(esp_err_t, device_iter_get_next,
                (onewire_device_iter_handle_t iter, onewire_device_t *dev),
                (override));
};

} // namespace ds18b20
