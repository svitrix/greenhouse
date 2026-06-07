# CLAUDE.md — `infrastructure/src/concurrency/`

> Shared types passed across FreeRTOS task boundaries (queues, stream buffers). Pure POD declarations — **no behaviour, no allocations**.

Currently empty — `SensorCache` is the only cross-task hand-off and it uses its own lock-free SPSC pattern. Add new POD queue payloads here when a use case actually needs one.

## Rules

- **POD only.** No constructors, no destructors, no virtual methods. These types must be trivially copyable so `xQueueSend` / `xQueueReceive` can `memcpy` them.
- **Fixed-width types.** `uint8_t`, `uint16_t`, `int16_t` — never `int` / `long` / `size_t` at queue boundaries.
- **Tagged unions, not `std::variant`.** `std::variant` carries an `index_` byte but also adds a destructor and conversion machinery — overkill for embedded SPSC queues. Use `enum class Kind : uint8_t` + `union {...}` like `SensorReading`.
- **Static `QueueHandle_t` only.** Queues themselves are created in the composition root (`main.cpp`) via `xQueueCreate(...)` and stored as `static`. Never `xQueueCreate` from inside a use case or driver.
- **Sized for steady-state load.** Pick queue depth based on the worst-case burst (e.g. 4 for sensor readings — one of each kind, two cycles deep).

## Adding a new queue payload

1. Define the POD struct here with `enum class Kind : uint8_t` + `union` if it carries variants.
2. Add a `static_assert(sizeof(MyPayload) <= 32, "Queue payload too large")` next to it — keeps queue memory budget visible.
3. The queue handle (`QueueHandle_t`) lives in `main.cpp`; pass it via reference / pointer to producers and consumers.
4. If the payload contains a pointer to anything heap-allocated — STOP. Queues copy by value; pointers across task boundaries are a lifetime nightmare. Restructure to embed the data inline or use a fixed pool.

## Not a place for…

- **FreeRTOS task wrappers / RTOS abstractions** — those don't exist in this project. Tasks are created directly in `main.cpp` with `xTaskCreate`. Adding an `ITaskRunner` would be premature abstraction.
- **Mutex / semaphore types** — none used yet (`SensorCache` is lock-free SPSC). If you need one, add it here as a wrapper that takes a `StaticSemaphore_t&` at construction.
