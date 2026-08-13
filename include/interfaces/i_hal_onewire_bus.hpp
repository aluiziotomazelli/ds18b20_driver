// components/ds18b20_driver/include/interfaces/i_hal_onewire_bus.hpp
#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"

#if !defined(CONFIG_IDF_TARGET_LINUX) && __has_include("onewire_bus.h")
#include "onewire_bus.h"
#include "onewire_bus_impl_rmt.h"
#include "onewire_device.h"
#include "onewire_types.h"
#else
struct onewire_bus_t;
typedef struct onewire_bus_t *onewire_bus_handle_t;
typedef uint64_t onewire_device_address_t;
struct onewire_device_iter_t;
typedef struct onewire_device_iter_t *onewire_device_iter_handle_t;

typedef struct {
    int bus_gpio_num;
    struct {
        uint32_t en_pull_up: 1;
    } flags;
} onewire_bus_config_t;

typedef struct {
    uint32_t max_rx_bytes;
} onewire_bus_rmt_config_t;

typedef struct onewire_device_t {
    onewire_bus_handle_t bus;
    onewire_device_address_t address;
} onewire_device_t;
#endif

namespace ds18b20 {

/**
 * @interface IOnewireBusHAL
 * @brief Interface for 1-Wire bus driver abstraction.
 */
class IOnewireBusHAL
{
public:
    virtual ~IOnewireBusHAL() = default;

    /** @copydoc onewire_new_bus_rmt() */
    virtual esp_err_t new_bus_rmt(const onewire_bus_config_t *bus_config,
                                 const onewire_bus_rmt_config_t *rmt_config,
                                 onewire_bus_handle_t *ret_bus) = 0;

    /** @copydoc onewire_bus_del() */
    virtual esp_err_t bus_del(onewire_bus_handle_t bus) = 0;

    /** @copydoc onewire_bus_reset() */
    virtual esp_err_t bus_reset(onewire_bus_handle_t bus) = 0;

    /** @copydoc onewire_bus_write_bytes() */
    virtual esp_err_t write_bytes(onewire_bus_handle_t bus,
                                 const uint8_t *tx_data,
                                 uint8_t tx_data_size) = 0;

    /** @copydoc onewire_bus_read_bytes() */
    virtual esp_err_t read_bytes(onewire_bus_handle_t bus,
                                uint8_t *rx_buf,
                                size_t rx_buf_size) = 0;

    /** @copydoc onewire_bus_write_bit() */
    virtual esp_err_t write_bit(onewire_bus_handle_t bus, uint8_t tx_bit) = 0;

    /** @copydoc onewire_bus_read_bit() */
    virtual esp_err_t read_bit(onewire_bus_handle_t bus, uint8_t *rx_bit) = 0;

    /** @copydoc onewire_new_device_iter() */
    virtual esp_err_t new_device_iter(onewire_bus_handle_t bus,
                                     onewire_device_iter_handle_t *ret_iter) = 0;

    /** @copydoc onewire_del_device_iter() */
    virtual esp_err_t del_device_iter(onewire_device_iter_handle_t iter) = 0;

    /** @copydoc onewire_device_iter_get_next() */
    virtual esp_err_t device_iter_get_next(onewire_device_iter_handle_t iter,
                                          onewire_device_t *dev) = 0;
};

} // namespace ds18b20
