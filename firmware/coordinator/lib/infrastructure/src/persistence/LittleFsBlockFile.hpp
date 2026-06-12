#pragma once
#include <LittleFS.h>
#include "IBlockFile.hpp"

namespace gh::infra {

// LittleFS-backed IBlockFile. Keeps a single File handle open for the
// object's lifetime (E4: one open handle instead of open/seek/close on
// every record) and mounts LittleFS lazily on first use.
class LittleFsBlockFile final : public IBlockFile {
public:
    explicit LittleFsBlockFile(const char* path) noexcept : path_{path} {}
    ~LittleFsBlockFile() override { if (file_) file_.close(); }

    [[nodiscard]] bool mount() noexcept {
        if (mounted_) return true;
        mounted_ = LittleFS.begin(/*formatOnFail=*/true);
        return mounted_;
    }

    [[nodiscard]] bool exists() const noexcept override {
        return LittleFS.exists(path_);
    }

    [[nodiscard]] bool create() noexcept override {
        if (file_) file_.close();
        File w = LittleFS.open(path_, "w");
        if (!w) return false;
        w.close();
        return reopen_();
    }

    void remove() noexcept override {
        if (file_) file_.close();
        LittleFS.remove(path_);
    }

    [[nodiscard]] size_t readAt(size_t offset, void* dst, size_t len) const noexcept override {
        if (!file_ && !const_cast<LittleFsBlockFile*>(this)->reopen_()) return 0;
        if (!file_.seek(offset)) return 0;
        return file_.read(static_cast<uint8_t*>(dst), len);
    }

    [[nodiscard]] size_t writeAt(size_t offset, const void* src, size_t len) noexcept override {
        if (!file_ && !reopen_()) return 0;
        if (!file_.seek(offset)) return 0;
        const size_t n = file_.write(static_cast<const uint8_t*>(src), len);
        file_.flush();
        return n;
    }

private:
    bool reopen_() noexcept {
        file_ = LittleFS.open(path_, "r+");
        return static_cast<bool>(file_);
    }

    const char*   path_;
    mutable File  file_{};
    bool          mounted_ = false;
};

}  // namespace gh::infra
