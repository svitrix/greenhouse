# Float Switch — Dry-run Guard (Coordinator)

> **⚠ STATUS: NOT INSTALLED.** The hardware float switch is not wired. The
> firmware currently runs with `FakeFloatSwitchAlwaysOk` (reports "wet" forever).
> [GPIO14](../../reference/canonical-values.md#coordinator-gpio) is **reserved**
> for it but not connected.

## Purpose

A float switch in the water reservoir is the **hardware** dry-run guard for the
pump: if the water level drops below the float, the switch opens and the firmware
must refuse to run the pump (and lock if running). It complements the software
`kPumpMaxRuntimeMs=20 s` watchdog — defence in depth against running the pump dry.

## Planned wiring (when installed)

| Property | Value |
|---|---|
| Pin | [GPIO14](../../reference/canonical-values.md#coordinator-gpio) (J1.12) — the only clean digital-only pin on J1 |
| Mode | `INPUT_PULLUP`, **active-low** (closed = wet = OK; open = dry) |
| Endpoint | float switch in the reservoir, other leg to GND |

```
GPIO14 ──┬── float switch ── GND
         └── (internal pull-up)
```

## Firmware reality

- Composition root (`coordinator/src/main.cpp`) wires `FakeFloatSwitchAlwaysOk`,
  which always reports wet. So **the dry-run guard is presently a no-op** — only
  the 20 s runtime cutoff actually protects the pump.
- `IrrigationServiceV2` already consumes an `IFloatSwitch` port, so installing the
  real switch is a composition-root swap (`GpioFloatSwitch{14}` instead of the
  fake) — no service changes.

## To bring into production

1. Wire the switch to GPIO14 + GND as above.
2. Replace `FakeFloatSwitchAlwaysOk` with the real GPIO adapter in `main.cpp`.
3. Update this page's status banner to **INSTALLED** and the
   [connection matrix](../../connections/coordinator-matrix.md) row to *wired*.
4. Bench-test: lift the float → pump must refuse to start / cut off + `SafetyLocked`.
