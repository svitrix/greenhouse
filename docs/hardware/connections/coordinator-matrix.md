# Connection Matrix — Coordinator (ESP32-C6-DevKitM-1)

> **What connects to what** on the coordinator, in one place. Pin numbers are
> mirrored from [canonical-values.md](../reference/canonical-values.md#coordinator-gpio)
> — that table is authoritative; this page shows the *wiring*.
>
> Board pin layout & gotchas: [DevKitM-1 board page](../boards/esp32-c6-devkitm-1/README.md).
> Power rails (12 V / 5 V / 3V3): [coordinator power](../power/coordinator-power.md).
> Assembly order: [coordinator-assembly.md](../coordinator-assembly.md).

## Power nets

| Net | Source | Feeds | Notes |
|---|---|---|---|
| **12 V** | USB-PD trigger (locked to 12 V via S1) | pump (+) through relay COM1; buck input | pump draws ~1.7–2 A peak — fed direct, **not** through the buck |
| **5 V** | buck 12 V→5 V (MINI-560) | ESP32 VIN (J1.14); relay coil VCC | |
| **3V3** | ESP32 onboard LDO (3V3 header) | I²C pull-ups; sensor VCC (if local sensors wired) | I/O reference — 3.3 V max on any GPIO |
| **GND** | common | everything | tie buck GND, relay GND, ESP32 GND (J1.13 + J3.1/J3.12) |

## Signal matrix (GPIO → net → endpoint)

| GPIO | Header | Net / signal | Connects to | Inline part | Direction |
|---|---|---|---|---|---|
| **GPIO6** | J1.10 | I²C SDA | (local sensor bus, idle in multi-node mode) | 4.7 kΩ pull-up → 3V3 | bidir |
| **GPIO7** | J1.11 | I²C SCL | (local sensor bus, idle) | 4.7 kΩ pull-up → 3V3 | bidir |
| **GPIO18** | J3.9 | relay CH1 IN | relay module IN1 → pump | **4.7 kΩ pull-down → GND** (keeps pump OFF during reset high-Z) | out, active-high |
| **GPIO19** | J3.8 | relay CH2 IN | relay module IN2 (spare) | — | out, reserved |
| **GPIO14** | J1.12 | float switch | **NOT WIRED** — reserved | (would be `INPUT_PULLUP`, active-low) | in — see note |
| **GPIO8** | J1.9 | WS2812 status LED | onboard LED | — | out, board-driven |
| **GPIO9** | J3.11 | BOOT button | onboard button | — | in, strapping — no external load |
| **GPIO16/17** | J3.2/J3.3 | UART0 TX/RX | onboard USB-UART bridge → `Serial` logs | — | — |
| **GPIO12/13** | J3.14/J3.13 | native USB D−/D+ | USB-C | — | — |

### Relay module wiring

```
ESP32 GPIO18 ──┬── relay IN1
               └── 4.7 kΩ ── GND        (pull-down: relay off while pin is high-Z on reset)
relay VCC  ── 5 V
relay GND  ── GND
relay jumper ── HIGH-active position
relay COM1 ── 12 V (+)
relay NO1  ── pump (+)
pump (−)   ── 12 V GND
```

The relay is **active-high** (`kPumpRelayActiveHigh=true`); `RelayPump` sets the
pin LOW (safe) **before** `pinMode(OUTPUT)`, and the external pull-down holds the
relay off during the brief reset window. Details:
[relay-pump page](../actuators/relay-pump/README.md).

> **Float switch (GPIO14):** documented as the production dry-run guard but the
> hardware is **not installed** — firmware wires `FakeFloatSwitchAlwaysOk`. Treat
> the GPIO14 row as *reserved, not connected*. See
> [float-switch page](../actuators/float-switch/README.md).

## Cross-check

Every pin above must equal its
[canonical row](../reference/canonical-values.md#coordinator-gpio) and the
firmware constant named there. If they differ, the firmware wins — fix the
canonical table, then this matrix.
