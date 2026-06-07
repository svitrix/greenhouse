// Compilation-unit stub for the native test environment.
// The infrastructure library is platform-restricted (espressif32 only) so
// the PlatformIO LDF does not build it for `sensor-node-native`. This file
// pulls in the implementation directly so the linker resolves ZigbeeReportMapper.
#include "../../lib/infrastructure/src/network/ZigbeeReportMapper.cpp"  // NOLINT(*-build-include)
