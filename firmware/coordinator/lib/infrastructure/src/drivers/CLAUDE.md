# CLAUDE.md — `infrastructure/src/drivers/`

> GPIO-level adapters for physical actuators and digital inputs **owned by the coordinator board**. (Sensor drivers — AM2315C / Chirp — live on the sensor-node side after Phase B; they are NOT here.)

## What lives here

| File                                                              | Implements             | Purpose                                                                                                                                                                                                                  |
|-------------------------------------------------------------------|------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| [`RelayPump.{hpp,cpp}`](RelayPump.cpp)                            | `gh::domain::IPump`    | Drives the water-pump relay on GPIO18. Ctor enforces **safe-state first**: writes LOW (off) to the pin **before** `pinMode(OUTPUT)`, relying on the board's pull-down during the brief high-Z window after reset.        |
| [`FakeFloatSwitchAlwaysOk.hpp`](FakeFloatSwitchAlwaysOk.hpp)      | `gh::domain::IFloatSwitch` | Header-only stub that always reports "water present" (`isDry() == false`). Used in `runOperational()` until the float switch hardware is fully wired and tested. **Replace with a real `GpioFloatSwitch` in Phase B**. |

## Conventions

- **Active-high vs active-low is explicit.** `RelayPump` ctor takes `bool active_high` (defaulted from `AppConfig::kPumpRelayActiveHigh`). Never assume polarity from the GPIO state alone.
- **Driver state is minimal.** A driver owns the pin number and at most one boolean. Higher-level safety (max runtime, lockout) lives in `application/IrrigationService`, not here.
- **No `delay()`.** A driver must return promptly. Long operations are the use-case layer's problem.
- **No `Serial.print` from inside a driver.** Pass an `ILogger&` if logging is genuinely needed (`RelayPump` currently doesn't — by design).

## Testing

- Host-side: `test_relay_pump` under `coordinator-native` uses an `IGpio` fake injected through the ctor — see [`test/test_relay_pump/`](../../../../test/test_relay_pump/). The fake records every `digitalWrite()` and `pinMode()` call so safe-state ordering can be asserted.
- On-target: no dedicated hwtest yet. Manual verification: probe GPIO18 with a multimeter during boot — must read 0 V continuously, never spike high before `RelayPump` ctor runs.

## Where the AM2315C / Chirp drivers went

In Phase A those drivers lived here. In Phase B they moved to [`firmware/sensor-node/lib/infrastructure/src/drivers/`](../../../../../sensor-node/) because the I²C bus is physically on the sensor-node. The coordinator now receives temperature / humidity / soil values via `ZigbeeCoordinatorAdapter` callbacks into `SensorCache`. **Do not re-introduce I²C sensor drivers in this directory** unless the hardware topology changes.

## Adding a new driver

1. Confirm there is an `I*` port in `shared/domain/src/ports/`. If not — add it there first.
2. Create `<Name>.{hpp,cpp}` here.
3. Take all hardware references via the ctor (`uint8_t gpio`, `TwoWire& bus`, `uint8_t i2c_addr`).
4. Write the safe-state preamble in the ctor for any output-capable pin.
5. Add a host test under `test/test_<name>/` with a fake `IGpio` / `ITwoWire` adapter.
