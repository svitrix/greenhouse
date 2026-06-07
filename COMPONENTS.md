# Greenhouse ESP32 Project Components

Summary of 21 items from [items.txt](items.txt) — key specifications, role in the irrigation system, and important assembly constraints.

> ⚠️ **This is the BOM (procurement + per-part gotchas), not the source of truth for pins.**
> Specific pins / I²C addresses / bus frequencies / calibration live **only** in
> [`docs/hardware/reference/canonical-values.md`](docs/hardware/reference/canonical-values.md).
> Pin numbers mentioned in the text below are illustrative; when in doubt,
> trust the canonical table (it is verified against the firmware). "What connects to what" —
> [`docs/hardware/connections/`](docs/hardware/connections/).

> **Architecture in brief:** USB-C PD → 12V → pump (via relay) + buck 5V → ESP32-C6 + relay coils. ESP32-C6 reads I2C sensors (air + soil), controls the relay. Firmware — custom Arduino-ESP32 on PlatformIO (following the [svitrix-firmware](https://github.com/svitrix/svitrix-firmware) pattern). Home Assistant integration — **via MQTT with HA Discovery**. In parallel — a custom **REST API** on ESPAsyncWebServer for direct control and automation.

---

## #1 — Breadboard 400 points (Transparent)

| Parameter | Value |
|---|---|
| Size | 84 × 55 × 8.5 mm |
| Points | 400 (300 signal + 100 power) |
| Pitch | 2.54 mm |
| Wire AWG | 20–29 |

**Role:** mounting platform for the prototype assembly. ESP32-C6 sits across the center gap; power rails are used as 5V (top +), 3.3V (bottom +), GND (top − and bottom − bridged).

**Gotchas:** the adhesive backing on the bottom — only stick it down once the layout is finalized.

---

## #2 — DuPont Jumper M-M 20 cm (×10)

Male–male jumper wires 200 mm, square pin 2.54 mm, stranded core.

**Role:** jumpers on the breadboard itself — ESP32 ↔ rails, rail-to-rail bridges, pull-ups.

---

## #3 — Espressif ESP32-C6-DevKitM-1

| Parameter | Value |
|---|---|
| MCU | ESP32-C6 RISC-V, 160 MHz, 1 core |
| RAM / Flash | 512 KB / 4 MB |
| Power | 5V (USB-C or VIN), internal LDO → 3.3V |
| GPIO | 22 |
| I/O levels | **3.3V** (max 3.3V on input — critical!) |
| Connectivity | Wi-Fi 6, BT 5.3 LE, **Zigbee 3.0, Thread, Matter** |
| I2C / SPI / UART | 2 / 1 / 3 interfaces |
| On-board | WS2812 RGB LED, BOOT button, RST button |
| USB | USB-C, CP2102 USB-Serial built-in |

**Role:** the brain of the system. Programmed via USB-C using PlatformIO + Arduino-ESP32. Wi-Fi connection, MQTT client (`AsyncMqttClient` or `PubSubClient`) for HA with HA Discovery, and a parallel HTTP server (`ESPAsyncWebServer`) for the REST API.

**Gotchas:**
- **GPIO0/1** — UART0, do not use for loads.
- **GPIO12/13** — USB D+/D−, occupied.
- **GPIO8** — on-board WS2812 (status LED).
- **GPIO9** — BOOT pin, do not load it.
- Source/sink max **40/28 mA per pin** — connect relays via optocoupler, not directly through a transistor base.
- In `platformio.ini`: `platform = espressif32@^6.x`, `board = esp32-c6-devkitm-1`, `framework = arduino`. Partitions — custom with dual OTA + LittleFS (see the spec).

---

## #4 — ASAIR AM2315C I2C Temperature/Humidity Sensor

| Parameter | Value |
|---|---|
| Power | 2.2–5.5 V |
| Interface | I2C (0x38, fixed address) |
| Built-in pull-up | **None** |
| Temperature | −40…+80 °C, ±0.3 °C, 0.01 °C res. |
| Humidity | 0–100 %RH, ±2 %, 0.024 % res. |
| Housing | IP-rated, 60 cm cable |

**Role:** air monitoring inside the greenhouse. Publishes `temperature` and `humidity` to HA.

**Gotchas:**
- **Power strictly at 3.3V**, not 5V. SDA/SCL are driven by the chip at VCC level — 5V would reach the ESP32 GPIO → damage it.
- No pull-ups — **mandatory 4.7 kΩ on SDA and SCL** to 3V3.
- Driver: Adafruit AHTX0 or a custom I2C read (address 0x38, command `0xAC 0x33 0x00` to trigger measurement, ~80 ms wait, 6-byte response). AM2315C is functionally compatible with AHT20.

---

## #5 — Brushless Water Pump 12V

| Parameter | Value |
|---|---|
| Power | 12 V DC, min. 1.7 A |
| Power (max) | up to 20 W |
| Flow rate | up to 12 L/min |
| Head | up to 6 m |
| Connection | G1/2" external thread |
| Motor type | Brushless DC (built-in driver) |
| Service life | 20 000 h |
| Direction | **Unidirectional** |
| Medium | **Clean water only** |

**Role:** pumps water from the reservoir into the drip line. Activated via relay CH1.

**Gotchas:**
- **Do not run dry** — centrifugal pump, will burn out within minutes without water. Protection: float switch + firmware watchdog (20 s max runtime).
- 1.7 A inrush current — power **directly from 12V via the PD trigger**, bypassing the buck (the buck won't handle it with margin).
- Polarity matters — observe `+/−` against the driver wires.

---

## #6 — USB PD Trigger Module

| Parameter | Value |
|---|---|
| Support | USB PD, QC, FCP, AFC |
| Output voltages | 5 / 9 / 12 / 15 / 20 V |
| Selection | button + lock via pad |
| Input / Output | USB-C / screw terminal |

**Role:** extract **12V** from a USB-PD power supply. A single USB-C cable for the whole system instead of a separate 12V adapter.

**Gotchas:**
- **Lock to 12V via S1** with a soldered jumper — otherwise an accidental reset may output 20V and destroy the buck/pump.
- Not all power supplies support 12V — verify beforehand (if the 12V LED blinks — the PSU cannot do 12V, use a different one).
- Use a PSU with **at least 25 W at 12V** (i.e., ≥2 A @ 12V) to start the pump.

---

## #7 — DuPont Jumper M-F 100 cm (×10)

Male–female jumper wires 1 m long.

**Role:** long reaches — from the breadboard to sensors, to the float switch in the tank, to the relay module if mounted separately.

---

## #8 — DC-DC Step-down Buck MINI 560 PRO (12V → 5V)

| Parameter | Value |
|---|---|
| Input | 7–32 V |
| Output | 5 V fixed |
| Current | up to 5 A with heatsink, **2.5 A without** |
| Power | up to 25 W |
| Protection | temperature, short-circuit, overload |

**Role:** provides 5V for the ESP32-C6 (via VIN) and for the relay coils from the 12V PD trigger.

**Gotchas:**
- Without a heatsink keep below **2.5 A** — ESP32 + 2 relays draw a peak of ~300 mA, plenty of margin.
- **Do not attempt to power the pump through the buck** — pump peak current > 1.7 A and the inrush is even higher, may trip the protection.
- Minimum input-to-output differential is 2 V — 12V→5V is fine.

---

## #9 — Chirp! Capacitive Soil Moisture Sensor (I2C)

| Parameter | Value |
|---|---|
| Power | 3.3–5 V |
| Interface | I2C (0x20 default, **changeable in software**) |
| Built-in pull-up | **None** |
| Measurement type | Capacitive (corrosion-resistant) |
| Cable | ~1 m |
| Colors | Red=VCC, Black=GND, Blue=SDA, Yellow=SCL |
| Extra | built-in thermistor |

**Role:** soil moisture sensor in the plant pot. Irrigation trigger in HA.

**Gotchas:**
- Power at **3.3V** for level compatibility with ESP32.
- No pull-ups — **the same 4.7 kΩ as for AM2315C** work on the shared bus.
- Address is changeable in software — if multiple Chirp! sensors are added in the future, they can be assigned different addresses.
- Calibration is mandatory: measure values in air (dry) and in a glass of water (wet), store in NVS/LittleFS as `soil_dry`/`soil_wet`, use linear interpolation in the driver.
- Driver — a custom thin implementation (not Apollon77). Design in `docs/superpowers/specs/2026-05-15-chirp-soil-driver-design.md`, decision history in `docs/decisions/2026-05-15-chirp-soil-driver.md`.
- Registers (full map in `archive/i2c-moisture-sensor-master/src/main.c:26-35`):
  `0x00 GET_CAPACITANCE r2` · `0x01 SET_ADDRESS w1` (double write on FW≥0x26) ·
  `0x02 GET_ADDRESS r1` · `0x03 MEASURE_LIGHT w0` · `0x04 GET_LIGHT r2` ·
  `0x05 GET_TEMPERATURE r2 (signed)` · `0x06 RESET w0` ·
  `0x07 GET_VERSION r1` · `0x08 SLEEP w0` · `0x09 GET_BUSY r1`.

---

## #10 — Resistor Set 10Ω–1MΩ (1% Metal Film, 1/4W)

20 values from 10 Ω to 1 MΩ, ~20 pcs each.

**Role in the project:**
| Value | Where | Purpose |
|---|---|---|
| 4.7 kΩ × 2 | SDA, SCL → 3V3 | I2C pull-up |
| 1 kΩ | base BC547 (#20) | Token-resistor for relay buffer |
| 4.7 kΩ | collector BC547 (#20) → 5V | Pull-up for open-collector output to relay IN |
| 10 kΩ | (optional) on float-switch | If internal INPUT_PULLUP is insufficient |

---

## #11 — 5V Relay 2-Channel (High-active or Low-active)

| Parameter | Value |
|---|---|
| Coil power (DC+) | 5 V, ~70 mA/channel |
| Signal input | 3.3–5 V |
| Channels | 2 |
| Contacts | NO + NC + COM |
| Switching | up to 250VAC / 30VDC, 10A |
| Mode | switchable via jumper: HIGH/LOW-active |

**Role:** CH1 — pump control, CH2 — spare.

**Gotchas:**
- **Jumper in HIGH-active** — in LOW-active mode the signal pin sees ~5V, which is dangerous for 3.3V GPIO.
- Pump wiring: 12V+ → COM1, NO1 → pump+. Pump− → GND.
- If HIGH-active does not trigger from 3.3V (happens on clones) — add an NPN transistor **BC547 (#20)** between GPIO and IN: GPIO→base via 1 kΩ, emitter→GND, collector→IN, + 4.7 kΩ pull-up from collector to 5V. Logic is inverted in firmware via `inverted: true`. For **non-inverted** operation — a BC547 → BC557 (#21) cascade, giving a 3.3V → 5V level-shifter while preserving polarity.
- **Switches an inductive load** (BLDC pump with driver) — the module already has a flyback diode on the relay coil, but optionally add a **TVS diode 18V** in parallel with the pump to suppress transients (optional, not critical).

---

## #12 — DuPont Jumper M-F 30 cm (×10)

**Role:** medium reach — from the breadboard to the nearby relay module, to the AM2315C sensor (60 cm stock cable + 30 cm extension).

---

## #13 — DuPont Jumper M-M 30 cm (×10)

**Role:** longer jumpers within the breadboard, cross-connections spanning the full board.

---

## #14 — DuPont Jumper F-F 20 cm (×10)

**Role:** connecting the relay module to the power screw terminals (12V from PD trigger, GND bus), pump wire extensions if needed.

---

## #15 — Murata 18650 Li-ion Battery US18650VTC6 (×2)

| Parameter | Value |
|---|---|
| Chemistry | Li-ion |
| Form factor | 18650 (flat top) |
| Capacity | 3120 mAh |
| Nominal / min / max voltage | 3.6 V / 2.0 V / 4.2 V |
| Max continuous discharge current | 30 A |
| Dimensions / weight | 18 × 18 × 65 mm / 48 g |
| Protection circuit | **None** (raw cell, no BMS) |

**Role:** standalone power for the **sensor-node** on the ESP32-C6 SuperMini (see #16). The sensor-node is a Zigbee end-device that sends I²C sensor readings to the coordinator; it runs on battery between deep-sleep periods. One cell per device, the second as a spare.

**Gotchas:**
- **No protection circuit** — the LTH7R/TP4054 chargers on the expansion (#17) / SuperMini (#16) only cover charging (overvoltage up to 4.2 V, current-limit 100 mA), but **do not protect against deep discharge, short-circuit, or reverse polarity**. For that, install **1S protection (#19)** between the cell (#15) and the load — it provides discharge cutoff @ 2.45 V and overcurrent @ ~2.5 A. Additionally — undervoltage cutoff in firmware: read V_bat via a voltage divider on ADC, disable the load at < 3.0 V (before protection trips — soft shutdown).
- **Do not solder the cell directly to B+/B−** on the SuperMini/expansion — use a holder (#18) + protection (#19) + JST-PH on the expansion (#17). This allows cell replacement without desoldering.
- **Polarity is critical** when connecting holder → protection → expansion: reverse on the cell side will destroy the protection board, on the load side — the load. Cell — *first* connection, JST-PH to expansion — *last*.
- **Charge rate via LTH7R/TP4054 ≈ 31 hours** from empty (3120 mAh / 100 mA). For fast charging — an external charger (TP4056 module at ~1 A) connected directly to B+/B− on the protection (#19).
- **High discharge current of 30 A — overkill** for the sensor-node (consumption < 100 mA), but gives huge DOD headroom and long service life.
- Long-term storage: 30–50% charge at +15…+25 °C. Do not leave fully discharged for more than a few days — the cell degrades.
- **Do not use in the coordinator** — it is powered from the 5V buck (see #8) via the main USB-PD; battery backup for it is a separate task (optional, UPS module not included).

---

## #16 — ESP32-C6 SuperMini Development Board (×2)

| Parameter | Value |
|---|---|
| MCU | ESP32-C6FH4 RISC-V, 160 MHz, 1 HP core + 1 LP core |
| RAM / Flash | 512 KB SRAM / 4 MB SPI flash (in-package) |
| Size / weight | 26 × 18 mm / 3 g |
| Power | 5V USB-C (input) · internal LDO → **3V3 (OUT)** for loads |
| GPIO on pin headers | **22 pins:** `GP0..GP9` + `GP12..GP23` (no `GP10/GP11` — internal SPI flash) |
| "User-free" GPIO | **17** (after excluding `GP8` (LED), `GP12/GP13` (USB), `GP16/GP17` (UART0)) |
| Max I/O | 40 mA source / 28 mA sink per pin |
| ADC | **ADC1** on `GP0..GP6` (7 channels) · no ADC2 |
| LP domain | `GP0..GP7` (LP_GPIO, LP_UART RX/TX = GP4/GP5, LP_I2C SDA/SCL = GP6/GP7) |
| USB | native USB-C, Full-Speed 12 Mbps · USB-CDC + USB-JTAG on `GP12 (D−)` / `GP13 (D+)` |
| UART0 (logs) | `GP16 (TX)` · `GP17 (RX)` — exposed as dedicated "TX"/"RX" pins |
| Connectivity | Wi-Fi 6 (2.4 GHz), BT 5.3 LE, Zigbee 3.0, Thread, Matter |
| PWM / I²C / SPI / UART | up to 12 / 2 / 1 / 3 (via GPIO matrix — almost any pin) |
| On-board LEDs | **WS2812B RGB** on `GP8` (status) · **separate LED** on `GP15` · **Battery LED** (charge indicator from TP4054, no firmware control) |
| Battery charger | **TP4054** — Li-Po/Li-ion, 4.2 V max, 100 mA |
| Protection | **None** |
| Battery contact | solder pad (B+/B−) |
| Programming | USB-CDC via USB-C (USB-to-Serial integrated in MCU — no external bridge) |
| Pinout | [docs/hardware/boards/esp32-c6-supermini/pinout.jpg](docs/hardware/boards/esp32-c6-supermini/pinout.jpg) |
| Schematic | [docs/hardware/boards/esp32-c6-supermini/schematic.jpg](docs/hardware/boards/esp32-c6-supermini/schematic.jpg) |

**Role:** platform for **sensor-node**s — battery-powered Zigbee end-devices. One sensor-node per "plant": reads local I²C sensors (Chirp! soil + AM2315C air) and sends readings to the coordinator (#3) over Zigbee 3.0. Between measurements — deep-sleep on the LP-core. Firmware — `firmware/sensor-node/` (see [CLAUDE.md §2](CLAUDE.md)).

**Pin header layout** (see the [pinout diagram](docs/hardware/boards/esp32-c6-supermini/pinout.jpg)):

| Side | Pin column (top to bottom) | Notes |
|---|---|---|
| **Left** | `TX (GP16)` · `RX (GP17)` · `GP0` · `GP1` · `GP2` · `GP3` · `GP4` · `GP5` · `GP6` · `GP7` · `GP8` · `GP23` · `GP22` | ADC1 = `GP0..GP6` · LP domain = `GP0..GP7` · `GP8` = on-board RGB LED |
| **Right** | `5V` · `GND` · `3V3 (OUT)` · `GP20` · `GP19` · `GP18` · `GP15` · `GP14` · `GP9` · `GP12` · `GP13` · `GP21` | `GP9` = BOOT pin · `GP12/GP13` = USB D−/D+ · `GP15` = strap + separate user-LED |

**Gotchas:**
- **`GP10`, `GP11` are not exposed** — internal pins of the SPI flash module. Do not use in schematics.
- **`GP8` — on-board WS2812B status LED.** Cannot be used as a regular GPIO without giving up the LED. The sensor-node needs the status LED, so it stays assigned.
- **`GP15` — separate user-LED** (labeled "LED GP15" on the pinout). Also a **strapping pin** — do not attach a low-impedance load before the reset completes. Can be used as an indicator or left unused.
- **Battery LED** near the USB-C — charge indicator from TP4054, **not controlled by firmware** (lights up when charging voltage is applied and a cell is present).
- **`GP16/GP17` (TX/RX)** — UART0 for `Serial.print()` logs. **SuperMini has no separate USB-UART bridge** (unlike DevKitM-1 #3) — `Serial` logs go through native **USB-CDC**, so `GP16/GP17` are free for dedicated UARTs. If `Serial` is redirected to USB-CDC (`-DARDUINO_USB_CDC_ON_BOOT=1`), TX/RX can be assigned to a second full UART (e.g., for LP_UART interaction or Modbus).
- **`GP12/GP13` (USB D−/D+)** — **exposed on the pin header**, but physically connected to USB. Using them as GPIO requires fully abandoning USB → loses USB-C programming and USB-CDC logs (firmware would need to be flashed via UART0 on `GP16/GP17` with an external USB-UART adapter).
- **Strapping pins** are the same as on the DevKitM-1: `GP4, GP5, GP8, GP9, GP15` — all on the header. Do not attach low-impedance loads before the reset completes; `GP9` — **BOOT button** (Boot + Reset → Firmware Download mode).
- **ADC1** — only on `GP0..GP6` (7 channels). The only ADC on the chip; there is no ADC2.
- **LP domain** — `GP0..GP7`. I²C can be polled via LP_I2C (SDA=`GP6`, SCL=`GP7`) from deep-sleep through the LP-core without waking the HP-core. Critical for sensor-node power efficiency.
- **`3V3 (OUT)` on the right header** — internal LDO output; used to power external sensors. Current capacity is limited (typically ~300–500 mA accounting for Wi-Fi MCU peak up to ~300 mA); for heavy loads use an external LDO.
- **TP4054 at 100 mA charging** — slow but safe for VTC6 (#15). In the current build, charging goes through **LTH7R on the expansion (#17)**, not through the SuperMini's TP4054 — two linear chargers in parallel must not be used simultaneously.
- **No battery discharge protection** on the board — covered by **external 1S protection (#19)** between the cell (#15) and the expansion (#17). Additionally — undervoltage cutoff in firmware.
- **Differences from ESP32-C6-DevKitM-1 (#3):** the same chip ESP32-C6FH4, the same set of exposed GPIO (`GP0..9, 12..23` without `10/11`). Differences — form factor (26×18 vs ~26×54 mm), built-in TP4054 + B+/B− pads (absent on DevKitM-1), USB-CDC instead of a separate USB-UART bridge, separate LED on `GP15`. Strapping/ADC/LP mapping is identical.
- In `platformio.ini` for the sensor-node: `board = esp32-c6-devkitm-1` (no board-specific definition for SuperMini; the MCU is the same), enable USB-CDC with flags `-DARDUINO_USB_CDC_ON_BOOT=1 -DARDUINO_USB_MODE=1`, partitioning — the same.

---

## #17 — ESP32-C6 SuperMini Expansion Board (×2)

| Parameter | Value |
|---|---|
| Size / weight | 37 × 28 mm / 10 g |
| Power | 5–6 V DC |
| Battery charger | **LTH7R** — Li-Po/Li-ion, 4.2 V max, 100 mA |
| Protection | **None** (charger only) |
| Battery contact | **JST-PH male** + solder pads (B+/B−) |
| IO connectors | Pin header 2.54 mm: male + female (stacks with SuperMini) |
| Mounting holes | 2× ⌀ 2.1 mm |

**Role:** breakout board for the SuperMini (#16) — adds a **JST-PH battery connector** instead of manually soldering the cell to the tiny B+/B− pads. Full sensor-node power chain: cell (#15) → holder (#18) → protection (#19) → JST-PH → expansion (#17) → SuperMini (#16).

**Gotchas:**
- **LTH7R ≈ TP4054 clone** — same 100 mA charge current, no protection against deep discharge, short-circuit, or reverse polarity. Protection remains external (#19).
- **Charging goes through LTH7R on the expansion**, not **through TP4054 on the SuperMini** simultaneously — these are two linear chargers in parallel, which guarantees current drift and instability. Connect the cell only to the expansion.
- **Stack-up:** expansion male pin headers ↔ SuperMini female pin headers (or vice versa). Expansion 37×28 mm vs SuperMini 26×18 mm — the expansion is wider and overhangs the edges; account for this in the enclosure.
- **5–6 V supply** — power only from a stable 5V source (USB on the SuperMini or buck #8), not directly from 12V and not directly from the cell (cell gives 3.0–4.2 V).
- **JST-PH (2.0 mm pitch)** — standard connector for 1S Li-ion. **Polarity varies by vendor** — verify against the `+/−` markings on the board with a multimeter before the first connection; reverse polarity will destroy LTH7R and/or the SuperMini.

---

## #18 — 18650 Battery Holder (Leaf Spring, Wires, 1 cell) (×2)

| Parameter | Value |
|---|---|
| For cell | 18650, **flat top or button top** |
| Configuration | 1 cell |
| Contacts | leaf spring (no welding) |
| Output | loose wire |
| Size / weight | 78 × 22 × 21 mm / 8 g |
| Brand | Blossom |

**Role:** holder for the VTC6 (#15) — **allows cell replacement without soldering**. Holder wires connect to B+/B− on protection (#19), then P+/P− → JST-PH (hand-made or with a connector) → expansion (#17).

**Gotchas:**
- **VTC6 — flat top**, holder supports both variants (flat / button) via leaf springs — compatibility OK.
- **Loose wire without a connector** — strip the wire ends, solder/crimp to the protection board. Extensions / connectors — personal preference (JST-PH to the expansion for removability is recommended).
- **Do not leave the holder with a cell installed without protection (#19)** — a wire short-circuit → 30 A discharge from VTC6 → fire risk. VTC6 can deliver this current; a bare holder does not limit it.
- **Housing 78×22×21 mm** — plan the space in the sensor-node enclosure in advance, plus clearance for cell replacement.
- **Polarity:** standard — red = "+", black = "−", but not always consistent across vendors. Verify with a multimeter before the first solder.

---

## #19 — 1S Li-ion/Li-Po Protection Circuit (×2)

| Parameter | Value |
|---|---|
| Type | 1S (single cell, Li-ion / Li-Po) |
| Overcharge protection | 4.25 ± 0.05 V |
| Overcharge release | 4.23 ± 0.05 V |
| Discharge protection | 2.45 ± 0.1 V |
| Continuous current | **2 A max** |
| Overcurrent trip | ~2.5 A |
| Contacts | **B+ / B−** (cell side) · **P+ / P−** (load side) |
| Extra | pads for spot-welding the cell directly |

**Role:** closes the **critical gap** in the sensor-node power system present in both the SuperMini and the expansion: both have only a linear charger, **with no discharge protection, short-circuit protection, or overcurrent protection**. Installed between the holder (#18) and the expansion (#17):

```
cell+ ─ holder+ ─→ [B+]  protection  [P+] ─→ JST-PH+ → expansion (LTH7R + SuperMini)
cell− ─ holder− ─→ [B−]              [P−] ─→ JST-PH−
```

**Gotchas:**
- **2 A continuous / ~2.5 A trip** — sensor-node draws < 100 mA, huge margin. For the coordinator with the pump (1.7 A peak, > 2 A inrush) **this is unsuitable** — it will false-trip. The coordinator is not battery-powered in the current architecture.
- **Discharge cutoff 2.45 V** — disconnects the load on deep discharge; VTC6 absolute minimum is 2.0 V, leaving 0.45 V margin. Firmware must perform a soft-shutdown earlier (~3.0 V) so the protection does not trip under normal operation — it is an emergency cutoff.
- **Overcurrent ~2.5 A** — short-circuit protection. Trip → cell disconnects; reset is usually automatic after load removal; on some clones it requires applying charging voltage to P+/P− to unlock (typical behavior of DW01 + 8205A).
- **Mapping is strict:** `B+/B−` — to the cell (via holder), `P+/P−` — to the load. Reverse on the cell side destroys the protection and/or cell; on the load side — the load (SuperMini/expansion). **First** connection — cell to B+/B−, **last** — load to P+/P−.
- **Charging passes through P+/P−**: applying 4.2 V to P+/P− charges the cell via B+/B−. The LTH7R charger on the expansion does exactly this; an external charger (TP4056 module) also connects to the P-side.
- **Clone size ~10×5 mm** — can be soldered directly to the holder and wrapped in heat-shrink tubing.
- **Do not attempt spot-welding cells** at home — the pads are for industrial spot welding, not a soldering iron (the cell's high thermal mass draws heat away). Use the holder (#18).

---

## #20 — BC547 NPN Transistor, TO-92 (×2)

| Parameter | Value |
|---|---|
| Type | NPN BJT, general-purpose, small-signal |
| Package | TO-92 · **1 = Collector, 2 = Base, 3 = Emitter** (viewed from the marked face, flat side toward you) |
| V_CEO max | 45 V |
| V_CBO max | 50 V |
| V_EBO max | 6 V |
| I_C max (continuous) | **100 mA** |
| P_tot @ T_A = 25 °C | 625 mW |
| h_FE @ V_CE = 5 V, I_C = 2 mA | 110–800 (A: 110–220 · B: 200–450 · C: 420–800) |
| V_CE(sat) @ I_C = 100 mA, I_B = 5 mA | ≤ 0.3 V |
| V_BE(sat) @ I_C = 100 mA, I_B = 5 mA | ≤ 1.1 V (typical V_BE ≈ 0.7 V) |
| f_T | 150 MHz |
| T_J | −55…+150 °C |
| Complement | BC557 (#21) |
| Datasheet | [docs/hardware/datasheets/BC547.pdf](docs/hardware/datasheets/BC547.pdf) |

**Role:** spare NPN buffer between the ESP32-C6 GPIO (3.3 V) and the relay signal input (#11), in case the HIGH-active relay does not trigger from 3.3 V (IN threshold on clone modules can be ~3.5–4 V). Circuit — from gotcha #11: `GPIO → 1 kΩ → base` · `emitter → GND` · `collector → relay IN + 4.7 kΩ pull-up to 5V`. Logic is **inverted** (GPIO HIGH → transistor on → IN LOW → relay OFF) — compensated in firmware (e.g., `inverted: true` in HA).

**Gotchas:**
- **TO-92 pinout (1=C, 2=B, 3=E)** is read with the marked face toward you, flat side facing you. Confusing it with BC557 (#21) — both in the same package with similar markings — is easy; sort them into separate bags when unpacking.
- **I_C max = 100 mA — NOT suitable for directly driving the relay coil of module #11** (~70 mA @ 5V, right at the limit with no margin). BC547 is used only to **switch the signal input** IN (microamps), not the coil itself. For direct drive — a logic-level MOSFET (AO3400 / 2N7000) or an optocoupler.
- **Saturation requires β_sat ≈ 10**, not the datasheet h_FE. For I_C ≈ 1 mA (current through 4.7 kΩ pull-up to 5V) need I_B ≥ 0.1 mA → R_base ≤ (3.3 − 0.7) / 0.1 mA = 26 kΩ. **The 1 kΩ from table #10** gives I_B ≈ 2.6 mA — guaranteed deep saturation, V_CE(sat) close to 0.1 V.
- **V_BE ≈ 0.7 V** — factor this into base divider calculations and the turn-on threshold.
- **P_tot 625 mW at 25 °C** — derated at +85 °C it falls to ~250 mW. In a greenhouse enclosure in summer — monitor heat, but at I_C = 1 mA, V_CE_sat = 0.1 V → P_dis = 0.1 mW, enormous margin.
- **Do not use in ISR-critical circuits** without a pull-up — without a pull-up on the collector the ESP32 will read a floating input. With 4.7 kΩ pull-up — OK.

---

## #21 — BC557 PNP Transistor, TO-92 (×2)

| Parameter | Value |
|---|---|
| Type | PNP BJT, general-purpose, small-signal |
| Package | TO-92 · **1 = Collector, 2 = Base, 3 = Emitter** (same as BC547!) |
| V_CEO max | −45 V |
| V_CBO max | −50 V |
| V_EBO max | **−5 V** (note — less than NPN!) |
| I_C max (continuous) | **−100 mA** |
| P_tot @ T_A = 25 °C | 625 mW |
| h_FE @ V_CE = −5 V, I_C = −2 mA | 120–800 (A: 120–220 · B: 180–460 · C: 420–800) |
| V_CE(sat) @ I_C = −100 mA, I_B = −5 mA | ≤ −0.65 V |
| V_BE(sat) @ I_C = −100 mA, I_B = −5 mA | ≤ −1 V |
| f_T | 150 MHz |
| T_J | −55…+150 °C |
| Complement | BC547 (#20) |
| Datasheet | [docs/hardware/datasheets/BC557.pdf](docs/hardware/datasheets/BC557.pdf) |

**Role:** PNP complement to BC547 — for high-side switching and logic inverters. Use cases in the project:
- **3.3V → 5V level-shifter** for driving a HIGH-active relay **without logic inversion**: `GPIO 3.3V → BC547 (#20) → BC557 → relay IN 5V`. Double inversion = polarity preserved, GPIO HIGH → IN HIGH → relay ON.
- **High-side switch** for status LEDs and small 5V loads: `emitter → +5V` · `collector → load → GND` · `base via resistor from BC547 collector`.

**Gotchas:**
- **PNP is controlled by a LOW level on the base relative to the emitter.** For V_E = +5V, turning on the PNP requires V_B ≤ +4.3 V (V_BE ≤ −0.7 V); turning it off — V_B ≥ +5 V (V_BE ≈ 0).
- **V_EBO max = −5 V — tight margin!** If V_E = +5 V and the base is pulled directly to GND (3.3 V GPIO through a resistor → does not reach 0), V_BE = −5 V is already at the destruction threshold. **Correct pattern** — drive the PNP through an NPN buffer (BC547 #20) whose collector pulls the PNP base to GND, plus a base pull-up 4.7 kΩ to +5V for a guaranteed OFF when GPIO is high-impedance.
- **TO-92 pinout is identical to BC547** (1=C, 2=B, 3=E). Markings differ by one digit ("BC547" vs "BC557") — verify the package before soldering, especially if both are in the same bag.
- **I_C max = −100 mA** — same limits as the NPN; for higher currents — P-channel MOSFET (AO3401, logic-level).
- **V_CE(sat) = −0.65 V** — higher than NPN (−0.3 V). In a high-side switch configuration at +5V, the load receives ~4.35 V — account for this when calculating LED resistors; for 5V logic it may fall below V_IH.

---

## Power budget summary

| Rail | Source | Consumers | Margin |
|---|---|---|---|
| 12V / up to 3 A | USB-PD trigger | Pump (peak ~2 A), buck input | OK with PD PSU ≥45 W |
| 5V / up to 2.5 A | Buck MINI 560 | ESP32-C6 VIN, relay DC+ | Huge (max ~300 mA) |
| 3.3V / up to ~500 mA | ESP32-C6 LDO | AM2315C, Chirp!, pull-ups | ~50 mA actual — OK |
| GND | common | all | — |

## I2C bus summary

| Address | Device | VCC | Pull-up |
|---|---|---|---|
| 0x20 | Chirp! soil moisture | 3.3V | external 4.7 kΩ ✓ |
| 0x38 | AM2315C air T/RH | 3.3V | external 4.7 kΩ ✓ |

## ESP32-C6 pinout (in use)

| GPIO | Function | Connection |
|---|---|---|
| 14 | Float switch input | INPUT_PULLUP, active-low (digital-only pin, not strapping, not ADC, not LP — the only "clean" GPIO on J1 DevKitM-1, J1.12) |
| 6 | I2C SDA | both sensors + 4.7 kΩ → 3V3 (also `LP_I2C_SDA` — can be polled in deep-sleep via LP-core) |
| 7 | I2C SCL | both sensors + 4.7 kΩ → 3V3 (also `LP_I2C_SCL`) |
| 8 | WS2812 status LED | on-board |
| 18 | Relay CH1 → Pump | output, restore_mode OFF (J3.9 on DevKitM-1) |
| 19 | Relay CH2 → spare | output, restore_mode OFF (J3.8 on DevKitM-1) |
| 3V3 | sensor power | bottom power rail |
| 5V (VIN) | ESP32 power | from buck |
| GND × ≥2 | common ground | both rails |

> **GPIO10 and GPIO11 on DevKitM-1 are NOT exposed** on pin headers (internal pins of the MINI-1 module) — do not use. On DevKitC-1 they are exposed, but we have a different devkit.

---

## Not included in this order, but recommended

| Component | Purpose | Criticality |
|---|---|---|
| Float switch | Hardware protection against dry-running the pump | **Mandatory for production** |
| TVS diode P6KE18A or 1N5408 | Suppressing pump transients | Recommended |
| Silicone tubing 1/2" + drip line | Hydraulics | Mandatory |
| Reservoir (5–10 L) | Water source | Mandatory |
| USB-C PD adapter 45 W+ with 12V support | System power | Mandatory |
| IP54 enclosure for electronics | Moisture protection | Mandatory for production |

---

## Wokwi visualization

The file [diagram.json](diagram.json) is a **visual** wiring diagram for Wokwi. It is **not a runnable simulator** (Wokwi does not natively simulate AM2315C/Chirp!/buck/PD-trigger), but it shows physical connections for all 14 BOM items with substitutes and labels:

| Actual | In Wokwi | Label |
|---|---|---|
| #1 Breadboard 400 | `wokwi-breadboard-half` | real |
| #2/#7/#12/#13/#14 DuPont wires | the `connections[]` themselves | colors per legend |
| #3 ESP32-C6 | `board-esp32-c6-devkitm-1` | real |
| #4 AM2315C I2C | `wokwi-dht22` | labeled as AM2315C 0x38 |
| #5 BLDC pump 12V | `wokwi-led` + `wokwi-resistor` | labeled as PUMP 12V |
| #6 USB-PD trigger | `wokwi-vcc` + `wokwi-resistor` (label) | block with label |
| #8 Buck 12V→5V | `wokwi-resistor` (label) | block with label |
| #9 Chirp! I2C | `wokwi-potentiometer` | labeled as Chirp! 0x20 |
| #10 Resistors | 2× `wokwi-resistor` 4.7 kΩ + 1× 1 kΩ | real |
| #11 Relay 2-ch | `wokwi-relay-module` | real |
| (recommended) Float switch | `wokwi-pushbutton` | marked as to-buy |

To open: create a new project at wokwi.com → Files → `diagram.json` → replace contents → Save. Board id: `board-esp32-c6-devkitm-1`. If Wokwi does not recognize it — fall back to `board-esp32-c6-devkitc-1` or the nearest available ESP32 board.
