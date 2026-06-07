# Relay + Water Pump (Coordinator)

> Drives greenhouse irrigation. GPIO → relay → 12 V brushless pump. Pin/timeout
> values are mirrored from
> [canonical-values.md](../../reference/canonical-values.md#coordinator-gpio);
> wiring is in the [coordinator connection matrix](../../connections/coordinator-matrix.md#relay-module-wiring).
> Purchasing details: [COMPONENTS.md](../../../../COMPONENTS.md) #5, #11.

## Pump — Brushless 12 V DC

| Parameter | Value |
|---|---|
| Supply | 12 V DC, ≥1.7 A (peak ~2 A) |
| Power | up to 20 W |
| Flow / head | up to 12 L/min · up to 6 m |
| Motor | brushless (integrated driver), **one-directional** |
| Medium | **clean water only**, must stay submerged |

**Gotchas:**
- **Never run dry** — a centrifugal pump self-destructs in minutes without water.
  Two guards: the [float switch](../float-switch/README.md) (dry-run, *not yet
  installed*) and the firmware `kPumpMaxRuntimeMs=20 s` watchdog.
- Fed **directly from 12 V** (PD trigger), bypassing the buck — the buck cannot
  supply the inrush current.

## Relay module — 5 V, 2-channel

| Parameter | Value |
|---|---|
| Coil supply | 5 V, ~70 mA per channel |
| Channels | CH1 = pump ([GPIO18](../../reference/canonical-values.md#coordinator-gpio)), CH2 = spare (GPIO19) |
| Contacts | NO / NC / COM, 250 VAC / 30 VDC / 10 A |
| Signal level | 3.3–5 V if the jumper is set **HIGH-active** |

**Critical:** set the on-board jumper to **HIGH-active**, otherwise the polarity
is inverted and the firmware's safe-state logic energises the pump on reset.

## Firmware contract

- Constant: `kPumpRelayGpio=18`, `kPumpRelayActiveHigh=true`
  ([`AppConfig.hpp`](../../../../firmware/shared/application/src/AppConfig.hpp)),
  `kRelayIn1Pin=18`
  ([`CoordinatorConfig.hpp`](../../../../firmware/coordinator/lib/application/src/CoordinatorConfig.hpp)).
- Driver: `RelayPump` sets the pin to the safe level (LOW for active-high)
  **before** `pinMode(OUTPUT)`. The external **4.7 kΩ pull-down → GND** on IN1
  holds the relay off during the brief high-Z reset window.
- Safety state machine (`IrrigationServiceV2`): `Off → Running → SafetyLocked`.
  Lock triggers on `runtime ≥ 20 s` or float-switch dry. The 20 s cutoff is the
  last line of defence — **never disable it**; raise the constant if ever needed.

## Optional level-shift

If a relay clone has an IN threshold above 3.3 V and won't trigger from the
ESP32's 3.3 V GPIO, buffer it with a transistor — see
[BC547/BC557 page](../transistors-bc547-bc557/README.md). Not needed for a
correctly-jumpered HIGH-active module.
