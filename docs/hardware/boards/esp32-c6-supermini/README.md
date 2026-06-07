# ESP32-C6 SuperMini

> **★ This is the board running `firmware/sensor-node`.**
>
> Battery-powered Zigbee end device: one sensor-node per plant reads local
> I²C sensors ([Chirp! soil](../../sensors/chirp-soil-moisture/README.md) +
> [AM2315C air](../../sensors/am2315c/README.md)) and sends readings to the
> coordinator ([ESP32-C6-DevKitM-1](../esp32-c6-devkitm-1/README.md)) over Zigbee 3.0.
> Between measurements the device is in deep sleep on the LP core.

Board files in this folder:

- [`pinout.jpg`](pinout.jpg) — pin header layout
- [`schematic.jpg`](schematic.jpg) — board schematic

---

## Specifications

| Parameter | Value |
|---|---|
| MCU | ESP32-C6FH4 RISC-V, 160 MHz, 1 HP core + 1 LP core |
| RAM / Flash | 512 KB SRAM / 4 MB SPI flash (in-package) |
| Size / weight | 26 × 18 mm / 3 g |
| Power | 5 V USB-C (input) · internal LDO → **3V3 (OUT)** for external loads |
| GPIO on pin headers | **22 pins:** `GP0..GP9` + `GP12..GP23` (no `GP10/GP11` — internal SPI flash) |
| "User-free" GPIO | **17** (minus `GP8` LED, `GP12/GP13` USB, `GP16/GP17` UART0) |
| Max I/O | 40 mA source / 28 mA sink per pin |
| ADC | **ADC1** on `GP0..GP6` (7 channels) · no ADC2 |
| LP domain | `GP0..GP7` (LP_GPIO, LP_UART RX/TX = `GP4/GP5`, LP_I2C SDA/SCL = `GP6/GP7`) |
| USB | native USB-C, Full-Speed 12 Mbps · USB-CDC + USB-JTAG on `GP12 (D−)` / `GP13 (D+)` |
| UART0 (logs) | `GP16 (TX)` · `GP17 (RX)` — exposed as separate "TX"/"RX" pins |
| Connectivity | Wi-Fi 6 (2.4 GHz), BT 5.3 LE, Zigbee 3.0, Thread, Matter |
| PWM / I²C / SPI / UART | up to 12 / 2 / 1 / 3 (via GPIO matrix — almost any pin) |
| On-board LEDs | **WS2812B RGB** on `GP8` (status) · separate LED on `GP15` · Battery LED (TP4054, not firmware-controlled) |
| Battery charger | **TP4054** — Li-Po/Li-ion, 4.2 V max, 100 mA |
| Battery protection | **None** (covered by an external 1S-protection module) |
| Battery contact | solder pad (B+/B−) |
| Flashing | USB-CDC via USB-C (USB-to-Serial integrated in MCU) |

---

## Pin header layout

See [`pinout.jpg`](pinout.jpg).

| Side | Pin column (top to bottom) | Notes |
|---|---|---|
| **Left** | `TX (GP16)` · `RX (GP17)` · `GP0` · `GP1` · `GP2` · `GP3` · `GP4` · `GP5` · `GP6` · `GP7` · `GP8` · `GP23` · `GP22` | ADC1 = `GP0..GP6` · LP domain = `GP0..GP7` · `GP8` = on-board RGB LED |
| **Right** | `5V` · `GND` · `3V3 (OUT)` · `GP20` · `GP19` · `GP18` · `GP15` · `GP14` · `GP9` · `GP12` · `GP13` · `GP21` | `GP9` = BOOT pin · `GP12/GP13` = USB D−/D+ · `GP15` = strap + separate user LED |

### Sensor connections (sensor-node)

| GPIO | Function | Connection |
|---|---|---|
| `GP6` | I²C SDA (= `LP_I2C_SDA`) | Chirp! + AM2315C + 4.7 kΩ → 3V3 — can be polled from deep sleep via LP core |
| `GP7` | I²C SCL (= `LP_I2C_SCL`) | Chirp! + AM2315C + 4.7 kΩ → 3V3 |
| `GP8` | WS2812B status LED | on-board |
| `3V3 (OUT)` | sensor power | internal LDO output |

---

## Gotchas

- **`GP10`, `GP11` not exposed** — internal SPI flash pins. Do not use in schematics.
- **`GP8` — on-board WS2812B status LED.** Cannot be used as a plain GPIO without giving up the LED. The sensor-node needs the status LED — keep it.
- **`GP15` — separate user LED** (labelled "LED GP15"). Also a **strapping pin** — do not attach a low-impedance load until reset completes.
- **Battery LED** near the USB-C port — charge indicator from TP4054, **not firmware-controlled**.
- **`GP16/GP17` (TX/RX)** — UART0 for `Serial.print()`. The SuperMini has **no separate USB-UART bridge** — `Serial` logs go via native USB-CDC, so TX/RX are free for custom UARTs. Enable USB-CDC with `-DARDUINO_USB_CDC_ON_BOOT=1 -DARDUINO_USB_MODE=1`.
- **`GP12/GP13` (USB D−/D+)** — exposed on the header but physically tied to USB. Use as GPIO only if you fully abandon USB (losing firmware flashing and CDC logs via USB-C).
- **Strapping pins:** `GP4, GP5, GP8, GP9, GP15` — all on headers. `GP9` — BOOT button (Boot + Reset → Firmware Download mode).
- **ADC1** — only `GP0..GP6` (7 channels). No ADC2.
- **LP domain** `GP0..GP7` — I²C on LP_I2C (`GP6/GP7`) can be polled from deep sleep via the LP core without waking the HP core. Critical for sensor-node power efficiency.
- **`3V3 (OUT)`** — current budget limited (~300–500 mA accounting for Wi-Fi peak). For heavy loads use an external LDO.
- **TP4054 at 100 mA** — slow but safe. In the current build charging goes through the **LTH7R on the expansion board**, not TP4054 on the SuperMini — two linear chargers in parallel must not be used simultaneously.
- **No battery discharge protection** on the board — covered by an external **1S-protection** module between the cell and the expansion board. Additionally — firmware undervoltage cutoff (soft shutdown at ~3.0 V).

---

## Sensor-node battery chain

```
cell (VTC6) → holder → 1S-protection [B+/B−|P+/P−] → JST-PH → expansion (LTH7R) → SuperMini
```

Full specifications and gotchas for each link (cell #15, SuperMini #16, expansion
#17, holder #18, protection #19) — in [`COMPONENTS.md`](../../../../COMPONENTS.md).

---

## Differences from ESP32-C6-DevKitM-1

Same ESP32-C6FH4 chip, same exposed GPIO set (`GP0..9, 12..23`, without
`10/11`). Differences:

- form factor 26 × 18 mm (vs ~26 × 54 mm for DevKitM-1);
- built-in TP4054 + B+/B− pads (absent on DevKitM-1);
- USB-CDC instead of a separate USB-UART bridge;
- separate LED on `GP15`.

Strapping / ADC / LP map is identical. In `platformio.ini` for sensor-node:
`board = esp32-c6-devkitm-1` (no SuperMini-specific board definition exists,
same MCU).

---

## Flashing

```
pio run -e sensor-node          -d firmware/sensor-node
pio run -e sensor-node -t upload -d firmware/sensor-node
```

Architecture rules for this board — [`firmware/sensor-node/CLAUDE.md`](../../../../firmware/sensor-node/CLAUDE.md).
