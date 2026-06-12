#pragma once

// Portable recursive mutex + RAII scoped lock.
//
// On ARDUINO (ESP32 / FreeRTOS) this wraps a statically-allocated
// FreeRTOS recursive mutex. On the native host build (no FreeRTOS) it
// degrades to a no-op: the native unit tests are single-threaded, so the
// lock has nothing to guard, and the registry/history .cpp files that
// include this header are compiled straight into the host test binaries.
//
// Recursive (not plain) so a public method may call another public method
// of the same object while already holding the lock without self-deadlock.

namespace gh::infra {

#ifdef ARDUINO

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class RecursiveMutex final {
public:
    RecursiveMutex() noexcept
        : handle_{xSemaphoreCreateRecursiveMutexStatic(&storage_)} {}

    RecursiveMutex(const RecursiveMutex&)            = delete;
    RecursiveMutex& operator=(const RecursiveMutex&) = delete;

    void lock()   noexcept { xSemaphoreTakeRecursive(handle_, portMAX_DELAY); }
    void unlock() noexcept { xSemaphoreGiveRecursive(handle_); }

private:
    StaticSemaphore_t  storage_{};
    SemaphoreHandle_t  handle_;
};

#else  // native host build — no FreeRTOS

class RecursiveMutex final {
public:
    RecursiveMutex() noexcept = default;
    RecursiveMutex(const RecursiveMutex&)            = delete;
    RecursiveMutex& operator=(const RecursiveMutex&) = delete;

    void lock()   noexcept {}
    void unlock() noexcept {}
};

#endif

class LockGuard final {
public:
    explicit LockGuard(RecursiveMutex& m) noexcept : m_{m} { m_.lock(); }
    ~LockGuard() noexcept { m_.unlock(); }

    LockGuard(const LockGuard&)            = delete;
    LockGuard& operator=(const LockGuard&) = delete;

private:
    RecursiveMutex& m_;
};

}  // namespace gh::infra
