// Pulls in the ZclSensorMapper implementation directly so the linker
// resolves it under coordinator-native (lib/infrastructure is gated to
// espressif32 + arduino in its library.json).
#include "../../lib/infrastructure/src/network/ZclSensorMapper.cpp"  // NOLINT(*-build-include)
