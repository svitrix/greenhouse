#pragma once

namespace gh::domain {

// One-shot upgrade/migration flags persisted across reboots.
// Used to gate run-once routines (e.g. a retained-topic cleanup pass) so they
// fire exactly once over the lifetime of the device.
struct IOneShotFlagStore {
    virtual ~IOneShotFlagStore() = default;

    // True if the flag identified by key has already been set.
    [[nodiscard]] virtual bool isSet(const char* key) noexcept = 0;

    // Persist the flag as set. Returns false if the underlying store failed
    // (caller may retry on the next pass).
    [[nodiscard]] virtual bool set(const char* key) noexcept = 0;
};

}
