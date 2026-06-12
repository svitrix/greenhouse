// Pulls in both registry + history implementations directly so the linker
// resolves them under coordinator-native (lib/infrastructure is gated to
// espressif32 + arduino in its library.json).
#include "../../lib/infrastructure/src/registry/InMemoryHistoryStore.cpp"  // NOLINT(*-build-include)
#include "../../lib/infrastructure/src/registry/InMemoryNodeRegistry.cpp"  // NOLINT(*-build-include)
