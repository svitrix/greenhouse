# Power Chain — Coordinator (mains)

> How the coordinator is powered. One USB-C cable in, two rails out (12 V pump,
> 5 V logic). Purchasing: [COMPONENTS.md](../../../COMPONENTS.md) #6, #8.
> Wiring/assembly order: [coordinator-assembly.md](../coordinator-assembly.md).

```
USB-C PD supply ──► USB-PD trigger (locked 12 V) ──┬──► 12 V ──► pump (+) via relay COM1
                                                   └──► buck 12→5 V ──┬──► ESP32 VIN (J1.14)
                                                                      └──► relay coil VCC
ESP32 onboard LDO ──► 3V3 ──► I²C pull-ups (+ local sensor VCC if wired)
```

## Stages

| Stage | Part | In → Out | Notes |
|---|---|---|---|
| Mains adapter | USB-C PD/QC charger (≥25 W @ 12 V) | wall → USB-C | must actually support 12 V PD; verify before relying on it |
| PD trigger | USB-PD trigger module | USB-C → 12 V | **lock to 12 V** by soldering the S1 pad — an accidental renegotiation to 20 V would destroy the buck and pump |
| Buck | MINI-560 PRO | 12 V → 5 V | feeds logic + relay coils; **not** the pump (inrush too high) |
| LDO | ESP32-C6 onboard | 5 V → 3.3 V | I/O reference, 3.3 V max on GPIO |

## Rails

| Rail | Feeds | Budget |
|---|---|---|
| 12 V | pump (~1.7–2 A peak, direct from PD trigger) | size adapter ≥ 2 A @ 12 V |
| 5 V | ESP32 VIN, relay coils (~70 mA/ch) | buck rated well above sum |
| 3V3 | I²C pull-ups, local sensors | onboard LDO |
| GND | common — tie buck, relay, ESP32 (J1.13 + J3.1/J3.12) | single ground |

## Gotchas

- Pump current is fed **direct from 12 V**, never through the buck.
- PD trigger **must** be locked to 12 V (S1) before connecting anything downstream.
- Mutually-exclusive ESP32 power inputs: USB-C **or** 5 V header — do not back-feed
  both. During bring-up power the ESP32 from USB-C for flashing; the buck-5 V path
  is for standalone operation.
