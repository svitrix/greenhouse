#pragma once
#include <cstddef>
#include <cstdint>

namespace gh::infra {

// Minimal random-access byte-file seam used by LittleFsTelemetryQueue.
//
// Exists so the queue's integrity logic (magic / version / CRC header,
// transactional append, ring math) is host-testable without LittleFS:
// the embedded build injects a LittleFS-backed adapter, the native tests
// inject an in-memory fake. Offsets are absolute from the start of the
// file. Reads/writes return the number of bytes actually transferred.
class IBlockFile {
public:
    virtual ~IBlockFile() = default;

    // True if the backing file already exists.
    [[nodiscard]] virtual bool exists() const noexcept = 0;

    // Create (truncate) the backing file to empty. Returns false on failure.
    [[nodiscard]] virtual bool create() noexcept = 0;

    // Remove the backing file. No-op if it does not exist.
    virtual void remove() noexcept = 0;

    [[nodiscard]] virtual size_t readAt(size_t offset, void* dst, size_t len) const noexcept = 0;
    [[nodiscard]] virtual size_t writeAt(size_t offset, const void* src, size_t len) noexcept = 0;
};

}  // namespace gh::infra
