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

TEST_F(Ds18b20DriverTest, InitSuccess)
{
    setup_successful_init();
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
    // Second init should immediately return ESP_OK without creating another bus
    EXPECT_EQ(driver_.init(), ESP_OK);
}

TEST_F(Ds18b20DriverTest, ReadTemperatureSuccessPositive)
{
    setup_successful_init();

    // Reset before convert
    EXPECT_CALL(mock_hal_, bus_reset(dummy_bus_))
        .WillOnce(Return(ESP_OK))
        .WillOnce(Return(ESP_OK));

    // Write Match ROM + Convert T
    EXPECT_CALL(mock_hal_, write_bytes(dummy_bus_, _, 10))
        .WillOnce(Return(ESP_OK))
        .WillOnce(Return(ESP_OK));

    // Scratchpad for +25.0625 °C (0x0191)
    // Bytes 0..7: 0x91, 0x01, 0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10
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

    // Scratchpad for -10.125 °C (-162 raw = 0xFF5E: LSB=0x5E, MSB=0xFF)
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

    uint8_t scratchpad[9] = {0x91, 0x01, 0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10, 0xAA}; // Invalid CRC 0xAA

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

    // After deinit, reading temperature should fail with INVALID_STATE
    float temp = 0.0f;
    EXPECT_EQ(driver_.read_temperature(&temp), ESP_ERR_INVALID_STATE);
}

TEST(Ds18b20HelpersTest, Crc8Calculation)
{
    // DS18B20 default power-on scratchpad:
    // 0x50, 0x05, 0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10 => CRC is 0x1C
    uint8_t power_on_scratchpad[8] = {0x50, 0x05, 0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10};
    EXPECT_EQ(Ds18b20Driver::calculate_crc8(power_on_scratchpad, 8), 0x1C);
}

TEST(Ds18b20HelpersTest, RawTemperatureConversion)
{
    // +125 °C (0x07D0)
    EXPECT_FLOAT_EQ(Ds18b20Driver::convert_raw_to_celsius(0xD0, 0x07), 125.0f);

    // +85 °C (0x0550 - power-on reset state)
    EXPECT_FLOAT_EQ(Ds18b20Driver::convert_raw_to_celsius(0x50, 0x05), 85.0f);

    // +25.0625 °C (0x0191)
    EXPECT_FLOAT_EQ(Ds18b20Driver::convert_raw_to_celsius(0x91, 0x01), 25.0625f);

    // 0 °C (0x0000)
    EXPECT_FLOAT_EQ(Ds18b20Driver::convert_raw_to_celsius(0x00, 0x00), 0.0f);

    // -0.5 °C (0xFFF8)
    EXPECT_FLOAT_EQ(Ds18b20Driver::convert_raw_to_celsius(0xF8, 0xFF), -0.5f);

    // -55 °C (0xFC90)
    EXPECT_FLOAT_EQ(Ds18b20Driver::convert_raw_to_celsius(0x90, 0xFC), -55.0f);
}
