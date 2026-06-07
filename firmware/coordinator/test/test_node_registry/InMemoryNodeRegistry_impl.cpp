// Pulls in the InMemoryNodeRegistry implementation directly so the linker
// resolves it under coordinator-native (lib/infrastructure is gated to
// espressif32 + arduino in its library.json).
#include "../../lib/infrastructure/src/registry/InMemoryNodeRegistry.cpp"  // NOLINT(*-build-include)
