// Pulls in the InMemoryHistoryStore implementation directly so the linker
// resolves it under coordinator-native (lib/infrastructure is gated to
// espressif32 + arduino in its library.json).
#include "../../lib/infrastructure/src/registry/InMemoryHistoryStore.cpp"  // NOLINT(*-build-include)
