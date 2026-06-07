// Forwarding translation unit: pulls the driver implementation into the
// test_relay_pump binary without requiring the Arduino-locked
// gh-infra-coordinator library. The #ifdef ARDUINO guard in RelayPump.cpp
// ensures the host build compiles cleanly.
#include "../../lib/infrastructure/src/drivers/RelayPump.cpp"  // NOLINT(build/include)
