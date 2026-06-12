#pragma once
#include "ports/IOneShotFlagStore.hpp"
#include <cstring>
#include <string>
#include <vector>

namespace gh::test {

class FakeOneShotFlagStore final : public gh::domain::IOneShotFlagStore {
public:
    std::vector<std::string> set_flags;
    int  set_calls   = 0;
    bool fail_on_set = false;  // simulate an NVS write failure

    bool isSet(const char* key) noexcept override {
        for (const auto& f : set_flags) {
            if (f == key) return true;
        }
        return false;
    }

    bool set(const char* key) noexcept override {
        ++set_calls;
        if (fail_on_set) return false;
        set_flags.emplace_back(key);
        return true;
    }
};

}
