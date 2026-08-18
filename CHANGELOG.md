# Changelog

All notable changes to the `ds18b20_driver` component will be documented in this file.

## [0.2.0] - 2026-08-18

### Changed
- Migrated `espressif/onewire_bus` dependency to ESP-IDF Component Manager with target filtering (`target != linux`).
- Removed `external/onewire_bus` git submodule and `.gitmodules`.

## [0.1.0] - 2026-08-13

### Added
- Initial implementation of the standalone C++ `ds18b20_driver` component.
- Dependency injection pattern: abstract `IDs18b20Driver` interface and 1-Wire hardware abstraction via `IOnewireBusHAL`.
- Concrete 1:1 hardware abstraction `OnewireBusHAL` wrapping Espressif's `onewire_bus` driver with RMT backend.
- Device discovery filtering by 1-Wire Family Code (`0x28`).
- Temperature conversion and reading in degrees Celsius with 12-bit two's complement decoding.
- Data integrity verification using standard Dallas CRC-8 checksum calculation over the 9-byte scratchpad.
- Configurable measurement resolution (9, 10, 11, and 12 bits) via `set_resolution()` and `Ds18b20Config::initial_resolution`.
- Dynamically adjusted non-blocking conversion delays (100 ms to 800 ms) tailored to active resolution.
- Comprehensive GoogleTest suite executing on Linux host (23 test cases).
- Shared mocks (`MockOnewireBusHAL` and `MockDs18b20Driver`) for component and application testing.
- Ready-to-use polling example in `examples/polling` with ASCII wiring diagram.
- Independent GitHub Actions CI workflows for firmware build (ESP32-C3) and host test execution with coverage.
