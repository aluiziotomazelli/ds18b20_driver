// components/ds18b20_driver/host_test/test_ds18b20_driver/main/test_ds18b20_driver.cpp
#include <cstring>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ds18b20_driver.hpp"
#include "mock_hal_onewire_bus.hpp"

using namespace ds18b20;
using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgPointee;

class Ds18b20DriverTest : public ::testing::Test
{
protected:
    MockOnewireBusHAL mock_hal_;
    Ds18b20Config config_{
        .gpio_num = 4,
        .max_rx_bytes = 10,
        .enable_pullup = true,
        .initial_resolution = Resolution::BITS_12,
    };
    Ds18b20Driver driver_{mock_hal_, config_};
    onewire_bus_handle_t dummy_bus_{(onewire_bus_handle_t)0x1234};
    onewire_device_iter_handle_t dummy_iter_{(onewire_device_iter_handle_t)0x5678};
    uint64_t valid_rom_address_{0x1122334455667728ULL}; // Family code 0x28 (byte 0)

    void setup_successful_init()
    {
        EXPECT_CALL(mock_hal_, new_bus_rmt(_, _, _))
            .WillOnce(DoAll(SetArgPointee<2>(dummy_bus_), Return(ESP_OK)));

        EXPECT_CALL(mock_hal_, new_device_iter(dummy_bus_, _))
            .WillOnce(DoAll(SetArgPointee<1>(dummy_iter_), Return(ESP_OK)));

        onewire_device_t dev{};
        dev.bus = dummy_bus_;
        dev.address = valid_rom_address_;

        EXPECT_CALL(mock_hal_, device_iter_get_next(dummy_iter_, _))
            .WillOnce(DoAll(SetArgPointee<1>(dev), Return(ESP_OK)));

        EXPECT_CALL(mock_hal_, del_device_iter(dummy_iter_))
            .WillOnce(Return(ESP_OK));

        ASSERT_EQ(driver_.init(), ESP_OK);
    }
};

TEST_F(Ds18b20DriverTest, InitSuccessDefault12Bit)
{
    setup_successful_init();
    EXPECT_EQ(driver_.get_resolution(), Resolution::BITS_12);
}

TEST_F(Ds18b20DriverTest, InitWithCustomInitialResolution)
{
    Ds18b20Config custom_cfg{
        .gpio_num = 4,
        .max_rx_bytes = 10,
        .enable_pullup = true,
        .initial_resolution = Resolution::BITS_9,
    };
    Ds18b20Driver custom_driver(mock_hal_, custom_cfg);

    EXPECT_CALL(mock_hal_, new_bus_rmt(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(dummy_bus_), Return(ESP_OK)));

    EXPECT_CALL(mock_hal_, new_device_iter(dummy_bus_, _))
        .WillOnce(DoAll(SetArgPointee<1>(dummy_iter_), Return(ESP_OK)));

    onewire_device_t dev{};
    dev.bus = dummy_bus_;
    dev.address = valid_rom_address_;

    EXPECT_CALL(mock_hal_, device_iter_get_next(dummy_iter_, _))
        .WillOnce(DoAll(SetArgPointee<1>(dev), Return(ESP_OK)));

    EXPECT_CALL(mock_hal_, del_device_iter(dummy_iter_))
        .WillOnce(Return(ESP_OK));

    // set_resolution calls bus_reset and write_bytes (13 bytes)
    EXPECT_CALL(mock_hal_, bus_reset(dummy_bus_))
        .WillOnce(Return(ESP_OK));

    EXPECT_CALL(mock_hal_, write_bytes(dummy_bus_, _, 13))
        .WillOnce(Return(ESP_OK));

    ASSERT_EQ(custom_driver.init(), ESP_OK);
    EXPECT_EQ(custom_driver.get_resolution(), Resolution::BITS_9);
}

TEST_F(Ds18b20DriverTest, InitBusCreationFailure)
{
    EXPECT_CALL(mock_hal_, new_bus_rmt(_, _, _))
        .WillOnce(Return(ESP_ERR_NO_MEM));

    EXPECT_EQ(driver_.init(), ESP_ERR_NO_MEM);
}

TEST_F(Ds18b20DriverTest, InitIteratorCreationFailure)
{
    EXPECT_CALL(mock_hal_, new_bus_rmt(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(dummy_bus_), Return(ESP_OK)));

    EXPECT_CALL(mock_hal_, new_device_iter(dummy_bus_, _))
        .WillOnce(Return(ESP_FAIL));

    EXPECT_CALL(mock_hal_, bus_del(dummy_bus_))
        .WillOnce(Return(ESP_OK));

    EXPECT_EQ(driver_.init(), ESP_FAIL);
}

TEST_F(Ds18b20DriverTest, InitNoDeviceFound)
{
    EXPECT_CALL(mock_hal_, new_bus_rmt(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(dummy_bus_), Return(ESP_OK)));

    EXPECT_CALL(mock_hal_, new_device_iter(dummy_bus_, _))
        .WillOnce(DoAll(SetArgPointee<1>(dummy_iter_), Return(ESP_OK)));

    EXPECT_CALL(mock_hal_, device_iter_get_next(dummy_iter_, _))
        .WillOnce(Return(ESP_ERR_NOT_FOUND));

    EXPECT_CALL(mock_hal_, del_device_iter(dummy_iter_))
        .WillOnce(Return(ESP_OK));

    EXPECT_CALL(mock_hal_, bus_del(dummy_bus_))
        .WillOnce(Return(ESP_OK));

    EXPECT_EQ(driver_.init(), ESP_ERR_NOT_FOUND);
}

TEST_F(Ds18b20DriverTest, InitIgnoresOtherFamilyCodesUntilDS18B20Found)
{
    EXPECT_CALL(mock_hal_, new_bus_rmt(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(dummy_bus_), Return(ESP_OK)));

    EXPECT_CALL(mock_hal_, new_device_iter(dummy_bus_, _))
        .WillOnce(DoAll(SetArgPointee<1>(dummy_iter_), Return(ESP_OK)));

    onewire_device_t other_dev{};
    other_dev.address = 0x00EEFFCCDDBBAA10ULL; // Family 0x10 (DS18S20)

    onewire_device_t ds18b20_dev{};
    ds18b20_dev.address = valid_rom_address_; // Family 0x28

    EXPECT_CALL(mock_hal_, device_iter_get_next(dummy_iter_, _))
        .WillOnce(DoAll(SetArgPointee<1>(other_dev), Return(ESP_OK)))
        .WillOnce(DoAll(SetArgPointee<1>(ds18b20_dev), Return(ESP_OK)));

    EXPECT_CALL(mock_hal_, del_device_iter(dummy_iter_))
        .WillOnce(Return(ESP_OK));

    EXPECT_EQ(driver_.init(), ESP_OK);
}

TEST_F(Ds18b20DriverTest, InitIdempotency)
{
    setup_successful_init();
    EXPECT_EQ(driver_.init(), ESP_OK);
}

TEST_F(Ds18b20DriverTest, SetResolutionSuccess)
{
    setup_successful_init();

    EXPECT_CALL(mock_hal_, bus_reset(dummy_bus_))
        .WillOnce(Return(ESP_OK));

    EXPECT_CALL(mock_hal_, write_bytes(dummy_bus_, _, 13))
        .WillOnce([](onewire_bus_handle_t, const uint8_t *buf, uint8_t size) {
            EXPECT_EQ(size, 13);
            EXPECT_EQ(buf[0], 0x55); // Match ROM
            EXPECT_EQ(buf[9], 0x4E); // Write Scratchpad
            EXPECT_EQ(buf[10], 0x4B); // TH
            EXPECT_EQ(buf[11], 0x46); // TL
            EXPECT_EQ(buf[12], static_cast<uint8_t>(Resolution::BITS_10)); // Config byte
            return ESP_OK;
        });

    EXPECT_EQ(driver_.set_resolution(Resolution::BITS_10), ESP_OK);
    EXPECT_EQ(driver_.get_resolution(), Resolution::BITS_10);
}

TEST_F(Ds18b20DriverTest, SetResolutionNotInitialized)
{
    EXPECT_EQ(driver_.set_resolution(Resolution::BITS_9), ESP_ERR_INVALID_STATE);
}

TEST_F(Ds18b20DriverTest, SetResolutionBusResetFail)
{
    setup_successful_init();

    EXPECT_CALL(mock_hal_, bus_reset(dummy_bus_))
        .WillOnce(Return(ESP_FAIL));

    EXPECT_EQ(driver_.set_resolution(Resolution::BITS_9), ESP_FAIL);
}

TEST_F(Ds18b20DriverTest, SetResolutionWriteBytesFail)
{
    setup_successful_init();

    EXPECT_CALL(mock_hal_, bus_reset(dummy_bus_))
        .WillOnce(Return(ESP_OK));

    EXPECT_CALL(mock_hal_, write_bytes(dummy_bus_, _, 13))
        .WillOnce(Return(ESP_FAIL));

    EXPECT_EQ(driver_.set_resolution(Resolution::BITS_9), ESP_FAIL);
}

TEST_F(Ds18b20DriverTest, ReadTemperatureSuccessPositive)
{
    setup_successful_init();

    EXPECT_CALL(mock_hal_, bus_reset(dummy_bus_))
        .WillOnce(Return(ESP_OK))
        .WillOnce(Return(ESP_OK));

    EXPECT_CALL(mock_hal_, write_bytes(dummy_bus_, _, 10))
        .WillOnce(Return(ESP_OK))
        .WillOnce(Return(ESP_OK));

    uint8_t scratchpad[9] = {0x91, 0x01, 0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10, 0x00};
    scratchpad[8] = Ds18b20Driver::calculate_crc8(scratchpad, 8);

    EXPECT_CALL(mock_hal_, read_bytes(dummy_bus_, _, 9))
        .WillOnce([&scratchpad](onewire_bus_handle_t, uint8_t *buf, size_t size) {
            std::memcpy(buf, scratchpad, size);
            return ESP_OK;
        });

    float temp = 0.0f;
    EXPECT_EQ(driver_.read_temperature(&temp), ESP_OK);
    EXPECT_NEAR(temp, 25.0625f, 0.001f);
}

TEST_F(Ds18b20DriverTest, ReadTemperatureSuccessNegative)
{
    setup_successful_init();

    EXPECT_CALL(mock_hal_, bus_reset(dummy_bus_))
        .WillOnce(Return(ESP_OK))
        .WillOnce(Return(ESP_OK));

    EXPECT_CALL(mock_hal_, write_bytes(dummy_bus_, _, 10))
        .WillOnce(Return(ESP_OK))
        .WillOnce(Return(ESP_OK));

    uint8_t scratchpad[9] = {0x5E, 0xFF, 0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10, 0x00};
    scratchpad[8] = Ds18b20Driver::calculate_crc8(scratchpad, 8);

    EXPECT_CALL(mock_hal_, read_bytes(dummy_bus_, _, 9))
        .WillOnce([&scratchpad](onewire_bus_handle_t, uint8_t *buf, size_t size) {
            std::memcpy(buf, scratchpad, size);
            return ESP_OK;
        });

    float temp = 0.0f;
    EXPECT_EQ(driver_.read_temperature(&temp), ESP_OK);
    EXPECT_NEAR(temp, -10.125f, 0.001f);
}

TEST_F(Ds18b20DriverTest, ReadTemperatureNullArg)
{
    EXPECT_EQ(driver_.read_temperature(nullptr), ESP_ERR_INVALID_ARG);
}

TEST_F(Ds18b20DriverTest, ReadTemperatureNotInitialized)
{
    float temp = 0.0f;
    EXPECT_EQ(driver_.read_temperature(&temp), ESP_ERR_INVALID_STATE);
}

TEST_F(Ds18b20DriverTest, ReadTemperatureBusResetFail)
{
    setup_successful_init();

    EXPECT_CALL(mock_hal_, bus_reset(dummy_bus_))
        .WillOnce(Return(ESP_FAIL));

    float temp = 0.0f;
    EXPECT_EQ(driver_.read_temperature(&temp), ESP_FAIL);
}

TEST_F(Ds18b20DriverTest, ReadTemperatureWriteBytesFail)
{
    setup_successful_init();

    EXPECT_CALL(mock_hal_, bus_reset(dummy_bus_))
        .WillOnce(Return(ESP_OK));

    EXPECT_CALL(mock_hal_, write_bytes(dummy_bus_, _, 10))
        .WillOnce(Return(ESP_FAIL));

    float temp = 0.0f;
    EXPECT_EQ(driver_.read_temperature(&temp), ESP_FAIL);
}

TEST_F(Ds18b20DriverTest, ReadTemperatureReadBytesFail)
{
    setup_successful_init();

    EXPECT_CALL(mock_hal_, bus_reset(dummy_bus_))
        .WillOnce(Return(ESP_OK))
        .WillOnce(Return(ESP_OK));

    EXPECT_CALL(mock_hal_, write_bytes(dummy_bus_, _, 10))
        .WillOnce(Return(ESP_OK))
        .WillOnce(Return(ESP_OK));

    EXPECT_CALL(mock_hal_, read_bytes(dummy_bus_, _, 9))
        .WillOnce(Return(ESP_FAIL));

    float temp = 0.0f;
    EXPECT_EQ(driver_.read_temperature(&temp), ESP_FAIL);
}

TEST_F(Ds18b20DriverTest, ReadTemperatureCrcMismatch)
{
    setup_successful_init();

    EXPECT_CALL(mock_hal_, bus_reset(dummy_bus_))
        .WillOnce(Return(ESP_OK))
        .WillOnce(Return(ESP_OK));

    EXPECT_CALL(mock_hal_, write_bytes(dummy_bus_, _, 10))
        .WillOnce(Return(ESP_OK))
        .WillOnce(Return(ESP_OK));

    uint8_t scratchpad[9] = {0x91, 0x01, 0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10, 0xAA}; // Invalid CRC

    EXPECT_CALL(mock_hal_, read_bytes(dummy_bus_, _, 9))
        .WillOnce([&scratchpad](onewire_bus_handle_t, uint8_t *buf, size_t size) {
            std::memcpy(buf, scratchpad, size);
            return ESP_OK;
        });

    float temp = 0.0f;
    EXPECT_EQ(driver_.read_temperature(&temp), ESP_ERR_INVALID_CRC);
}

TEST_F(Ds18b20DriverTest, DeinitSuccess)
{
    setup_successful_init();

    EXPECT_CALL(mock_hal_, bus_del(dummy_bus_))
        .WillOnce(Return(ESP_OK));

    EXPECT_EQ(driver_.deinit(), ESP_OK);

    float temp = 0.0f;
    EXPECT_EQ(driver_.read_temperature(&temp), ESP_ERR_INVALID_STATE);
}

TEST(Ds18b20HelpersTest, Crc8Calculation)
{
    uint8_t power_on_scratchpad[8] = {0x50, 0x05, 0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10};
    EXPECT_EQ(Ds18b20Driver::calculate_crc8(power_on_scratchpad, 8), 0x1C);
}

TEST(Ds18b20HelpersTest, RawTemperatureConversion)
{
    EXPECT_FLOAT_EQ(Ds18b20Driver::convert_raw_to_celsius(0xD0, 0x07), 125.0f);
    EXPECT_FLOAT_EQ(Ds18b20Driver::convert_raw_to_celsius(0x50, 0x05), 85.0f);
    EXPECT_FLOAT_EQ(Ds18b20Driver::convert_raw_to_celsius(0x91, 0x01), 25.0625f);
    EXPECT_FLOAT_EQ(Ds18b20Driver::convert_raw_to_celsius(0x00, 0x00), 0.0f);
    EXPECT_FLOAT_EQ(Ds18b20Driver::convert_raw_to_celsius(0xF8, 0xFF), -0.5f);
    EXPECT_FLOAT_EQ(Ds18b20Driver::convert_raw_to_celsius(0x90, 0xFC), -55.0f);
}

TEST(Ds18b20HelpersTest, ConversionDelays)
{
    EXPECT_EQ(Ds18b20Driver::get_conversion_delay_ms(Resolution::BITS_9), 100);
    EXPECT_EQ(Ds18b20Driver::get_conversion_delay_ms(Resolution::BITS_10), 200);
    EXPECT_EQ(Ds18b20Driver::get_conversion_delay_ms(Resolution::BITS_11), 400);
    EXPECT_EQ(Ds18b20Driver::get_conversion_delay_ms(Resolution::BITS_12), 800);
}
