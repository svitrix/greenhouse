# Power Chain — Sensor-node (battery)

> How a battery sensor-node is powered. Single 18650 cell, protected, charged via
> the expansion board. Purchasing: [COMPONENTS.md](../../../COMPONENTS.md)
> #15, #17, #18, #19. Board specifics:
> [SuperMini board page](../boards/esp32-c6-supermini/README.md).

```
18650 (VTC6) ──► 1S protection PCB ──► LTH7R charger (expansion board) ──► VBAT
   VBAT ──► SuperMini 5V/VBAT pad ──► onboard LDO ──► 3V3 ──(p-MOSFET gate, GPIO4)──► gated 3V3 ──► sensors
   VBAT ──► 100 kΩ / 100 kΩ divider ──► GPIO0 (ADC1_CH0)   battery sense
```

## Stages

| Stage | Part | Notes |
|---|---|---|
| Cell | Murata 18650 VTC6 (Li-ion, 4.2 V max) | high-drain cell |
| Protection | external 1S protection PCB | discharge cutoff ~2.45 V, OC trip ~2.5 A — the cell has none on-board |
| Charger | LTH7R on the expansion board | USB-C → cell. **The SuperMini's own TP4054 (100 mA) is NOT used** — two parallel linear chargers are not allowed |
| Regulation | SuperMini onboard LDO | VBAT → 3V3 |
| Sensor gate | p-MOSFET on [GPIO4](../reference/canonical-values.md#sensor-node-gpio) | switches the sensor 3V3 rail off in deep sleep |

## Battery sensing

- Divider: `R1=100 kΩ` (VBAT→ADC) + `R2=100 kΩ` (ADC→GND) → ÷2 into
  [GPIO0/ADC1_CH0](../reference/canonical-values.md#sensor-node-gpio).
- SoC is piecewise-linear in firmware (`BatteryMonitor::voltageToSocPct`):
  4.20 V = 100 %, 3.70 V = 50 %, 3.40 V = 5 %, 3.20 V = 0 %. No fuel-gauge IC.
- Soft-shutdown undervoltage cutoff handled in firmware (~3.0 V).

## Gotchas

- **Do not** charge through the SuperMini TP4054 and the LTH7R at the same time —
  pick one (we use LTH7R). See [COMPONENTS.md](../../../COMPONENTS.md) #17.
- The cell has **no on-board protection** — the external 1S PCB is mandatory.
- The sensor rail (and its I²C pull-ups) is **gated** — de-powered in deep sleep
  to save battery. See the
  [sensor-node connection matrix](../connections/sensor-node-matrix.md).
