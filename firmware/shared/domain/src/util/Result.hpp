#pragma once
#include "errors/ErrorCode.hpp"

namespace gh::domain {
template<class T>
struct Result {
    ErrorCode err;
    T         value;

    [[nodiscard]] constexpr bool ok() const noexcept {
        return err == ErrorCode::Ok;
    }

    static constexpr Result success(T v) noexcept {
        return Result{ErrorCode::Ok, v};
    }

    static constexpr Result failure(ErrorCode e) noexcept {
        return Result{e, T{}};
    }
};
}
