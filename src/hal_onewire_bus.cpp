// components/ds18b20_driver/src/hal_onewire_bus.cpp
#include "hal_onewire_bus.hpp"

#include "onewire_bus.h"
#include "onewire_bus_impl_rmt.h"
#include "onewire_device.h"

namespace ds18b20 {

esp_err_t OnewireBusHAL::new_bus_rmt(const onewire_bus_config_t *bus_config,
                                    const onewire_bus_rmt_config_t *rmt_config,
                                    onewire_bus_handle_t *ret_bus)
{
    return onewire_new_bus_rmt(bus_config, rmt_config, ret_bus);
}

esp_err_t OnewireBusHAL::bus_del(onewire_bus_handle_t bus)
{
    return onewire_bus_del(bus);
}

esp_err_t OnewireBusHAL::bus_reset(onewire_bus_handle_t bus)
{
    return onewire_bus_reset(bus);
}

esp_err_t OnewireBusHAL::write_bytes(onewire_bus_handle_t bus,
                                    const uint8_t *tx_data,
                                    uint8_t tx_data_size)
{
    return onewire_bus_write_bytes(bus, tx_data, tx_data_size);
}

esp_err_t OnewireBusHAL::read_bytes(onewire_bus_handle_t bus,
                                   uint8_t *rx_buf,
                                   size_t rx_buf_size)
{
    return onewire_bus_read_bytes(bus, rx_buf, rx_buf_size);
}

esp_err_t OnewireBusHAL::write_bit(onewire_bus_handle_t bus, uint8_t tx_bit)
{
    return onewire_bus_write_bit(bus, tx_bit);
}

esp_err_t OnewireBusHAL::read_bit(onewire_bus_handle_t bus, uint8_t *rx_bit)
{
    return onewire_bus_read_bit(bus, rx_bit);
}

esp_err_t OnewireBusHAL::new_device_iter(onewire_bus_handle_t bus,
                                        onewire_device_iter_handle_t *ret_iter)
{
    return onewire_new_device_iter(bus, ret_iter);
}

esp_err_t OnewireBusHAL::del_device_iter(onewire_device_iter_handle_t iter)
{
    return onewire_del_device_iter(iter);
}

esp_err_t OnewireBusHAL::device_iter_get_next(onewire_device_iter_handle_t iter,
                                             onewire_device_t *dev)
{
    return onewire_device_iter_get_next(iter, dev);
}

} // namespace ds18b20
