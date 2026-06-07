# Coordinator — step-by-step assembly

> Step-by-step instructions for assembling the coordinator module (ESP32-C6-DevKitM-1 + pump). Follow the phases **strictly in order**: each phase is verified with a multimeter before moving to the next. Assembly from scratch — ~45 minutes, re-verification — ~10 minutes.
>
> Related documents:
> - [connections/coordinator-matrix.md](connections/coordinator-matrix.md) — connection matrix "what connects to what"
> - [reference/canonical-values.md](reference/canonical-values.md) — pins / addresses / calibration (source of truth)
> - [COMPONENTS.md](../../COMPONENTS.md) — specifications and gotchas for each item
> - [CLAUDE.md](../../CLAUDE.md) — firmware architecture

---

## Before you start

### Tools
- Multimeter (required — for measuring V and continuity)
- Soldering iron + solder + flux (for the S1 jumper on the PD-trigger)
- USB-C PD adapter **≥ 45 W with 12 V support** (see [COMPONENTS.md #6](../../COMPONENTS.md))
- USB-C cable (data-capable, not charging-only)
- USB-C → USB-A/C cable for connecting ESP32 to a computer (separate, not the one from the PD adapter)
- Tweezers / needle-nose pliers

### Components used

| # | Component | Qty | Role in this assembly |
|---|---|---|---|
| #1  | Breadboard 400 points | 1 | mounting platform |
| #2/13 | DuPont M-M jumpers | ~12 | inside breadboard |
| #14 | DuPont F-F jumpers | ~6 | to pump, to relay, to PD-trigger terminals |
| #3  | ESP32-C6-DevKitM-1 | 1 | MCU |
| #5  | Brushless Pump 12 V | 1 | pump |
| #6  | USB-PD Trigger | 1 | USB-C → 12 V |
| #8  | Buck MINI 560 PRO | 1 | 12 V → 5 V |
| #10 | Resistor 4.7 kΩ | 1 (opt. 2) | pull-down IN1 (and opt. IN2) |
| #11 | Relay 2-channel 5 V | 1 | pump switching |

### 4 hardware safety rules

1. **Before any wiring change — disconnect USB-C from the PD-trigger.** Plug in USB-C only after all wire connections have been confirmed correct with a multimeter.
2. **Never run the pump dry** — submerge it in a cup of water before the first power-on. The BLDC will burn out within minutes without water cooling.
3. **ESP32 GPIO — 3.3 V logic.** The relay IN pins must see ≤ 3.3 V. Do not apply 5 V there.
4. **Everything on breadboard with DuPont first, then secure.** Do not solder anything permanently until the entire system has run for 30 minutes under load.

---

## Phase 0 — Module preparation (no power)

### 0.1 Lock the PD-trigger to 12 V

On the PD-trigger board, find the pad group (usually labelled `5V / 9V / 12V / 15V / 20V`) or a single point `S1` near the "12V" marking.

- Solder **only** the pad marked `12 V` (or bridge `S1` to 12 V according to the specific board's datasheet).
- **Verify visually:** no other voltage pad must be shorted.

> ⚠ Without this step, an accidental PD renegotiation could deliver 20 V → the Buck and potentially the pump will be destroyed.

### 0.2 Set the relay jumper to HIGH-active

The relay module has a triple pin header with a jumper labelled `HIGH / LOW` (sometimes `H.L`). Move the jumper to the **HIGH-active** position (`HIGH` side).

> Logic: HIGH-active = relay closes when `IN1 = HIGH (3.3 V)`. LOW-active is the inverse, and at ESP32 boot (`GPIO18` starts LOW) the relay would accidentally close.

### 0.3 Verify polarity with a multimeter

For each module, find and mark with a marker:
- **PD-trigger:** where `+12V` and `GND` are on the screw terminal (per the board's own markings).
- **Buck:** where `IN+`, `IN−`, `OUT+`, `OUT−` are (usually printed on the PCB next to the pads).
- **Relay:** which pin is `DC+`, which is `DC−`, which is `IN1`, `IN2` (labels on the pin header).
- **Pump:** wires are usually red = `+`, black = `−`. If in doubt — blow air into the nozzle to determine flow direction, then mark.

> ⚠ One polarity reversal on the pump and it will run "backwards" — pushing air through the valve. This is safe for the pump itself (the BLDC controller is protected), but it will not pump water.

---

## Phase 1 — Power chain (PD-trigger + Buck), no MCU and no relay

> Goal: confirm the power section delivers the correct voltages **before** inserting the expensive ESP32.

### 1.1 Place PD-trigger and Buck

- Position the PD-trigger next to the breadboard (its USB-C jack will face outward) or secure it so the terminals face the breadboard.
- Insert the Buck into the breadboard so input and output face opposite halves.

### 1.2 Route +12 V and GND rails on the breadboard

- Free "top +" breadboard rail → label `+12 V`.
- Free "bottom +" breadboard rail → label `+5 V` (Buck output will come here later).
- Connect **both** negative rails with **one jumper** — this is your common ground. Label `GND`.

### 1.3 Connect PD-trigger → Buck

Wires (F-F from PD-trigger terminals into breadboard, M-M inside breadboard):

| From | To | Color (recommended) |
|---|---|---|
| PD-trigger `+12V` | `+12V` breadboard rail | red (power) |
| PD-trigger `GND` | `GND` breadboard rail | black |
| `+12V` rail | Buck `IN+` | red |
| `GND` rail | Buck `IN−` | black |

### 1.4 Multimeter check **before applying power**

- Continuity (buzzer mode) between **`GND` rail** and **GND PD-trigger** → should beep.
- Continuity between **`GND` rail** and **Buck IN−** → should beep.
- Continuity between **`+12V` rail** and **PD-trigger `+12V`** → should beep.
- Continuity between **`+12V` rail** and **`GND` rail** → **must not** beep (if it beeps — there is a short somewhere, find it).

### 1.5 First power-on

Plug USB-C from the PD adapter into the PD-trigger.

### ✅ Check #1 — 12 V at PD-trigger output

Multimeter in DC V mode, 20 V range:
- Red probe → `+12V` rail.
- Black probe → `GND` rail.
- **Should read 11.8 – 12.2 V.**

If it reads 5 V — PD negotiation failed. Possible causes:
1. Adapter does not support 12 V (try a different one).
2. S1 lock not soldered (see step 0.1).
3. USB-C cable is charging-only with no CC line (try a different cable).

### ✅ Check #2 — 5 V at Buck output

- Red probe → Buck `OUT+`.
- Black probe → `GND` rail.
- **Should read 4.95 – 5.05 V.**

If it reads 12 V — Buck input and output are swapped. **Immediately unplug USB-C** and rewire.

### 1.6 Route +5 V rail

- Buck `OUT+` → `+5V` breadboard rail.
- Buck `OUT−` → `GND` rail.

Verify: `+5V` rail = 5 V relative to `GND` rail.

**Unplug USB-C before the next phase.**

---

## Phase 2 — Relay without pump and without ESP32

> Goal: confirm the relay physically clicks and switches 12 V to `NO1` before trusting it with the pump.

### 2.1 Connect the relay

| From | To | Purpose |
|---|---|---|
| Relay `DC+` | `+5V` rail | coil power |
| Relay `DC−` | `GND` rail | coil ground |
| Relay `COM1` (screw terminal) | `+12V` rail | CH1 power input |
| `IN1`, `IN2` | **leave unconnected for now** | — |
| `NO1`, `NC1`, `COM2`, `NO2`, `NC2` | **leave unconnected for now** | — |

### 2.2 Multimeter check before power

- `DC+` ↔ `GND` rail → **no beep** (no short).
- `COM1` ↔ `GND` rail → **no beep** (no short through the relay — it is open).

### 2.3 Apply power

Plug in USB-C.

### ✅ Check #3 — relay receives correct power

- `DC+` ↔ `DC−` → 5 V.
- `COM1` ↔ `GND` → 12 V.

### ✅ Check #4 — relay physically works

Take a short jumper wire (M-M ~5 cm):
- Insert one end into `DC+` (same 5 V node).
- **Briefly touch** the other end to `IN1`.

You should hear a **distinct click** + the CH1 LED on the relay module lights up.

At the same time with a multimeter:
- Before touching: `NO1` ↔ `GND` → ~0 V (contact open).
- While touching: `NO1` ↔ `GND` → 12 V (contact closed, COM1 connected to NO1).
- After removing the jumper: back to ~0 V.

If the click is muffled or intermittent — the coil is not getting the full 5 V. If there is no click — the jumper is not in HIGH-active position, or the relay is a clone with a threshold > 3.3 V (a BC547 buffer #20 is then needed, see [COMPONENTS.md:204](../../COMPONENTS.md#L204)).

**Unplug USB-C before the next phase.**

---

## Phase 3 — Install ESP32-C6, but do **not** connect IN1/IN2

> Goal: confirm the ESP32 boots correctly from 5 V and is accessible for flashing, before trusting it with the relay.

### 3.1 Insert ESP32 into the center split of the breadboard

So that the ESP32 USB-C port faces the edge of the breadboard (accessible from outside).

### 3.2 Power the ESP32

| From | To |
|---|---|
| `+5V` rail | ESP32 `5V / VIN` (J1.14) |
| `GND` rail | ESP32 `GND` (J1.13, **mandatory** at least 2 GND jumpers — J1.13 and J3.1 or J3.12) |

> ⚠ Do not connect `IN1`/`IN2` to GPIO18/GPIO19 yet. The relay must be electrically isolated from the ESP until flashing.

### 3.3 Multimeter check before power

- ESP32 `VIN` ↔ `GND` → no beep (the board has capacitors, but there must be no short).

### 3.4 Apply power

Plug USB-C **into the PD-trigger** (not into the ESP32 — leave its USB-C alone for now).

### ✅ Check #5 — ESP32 boots

- The red power LED and **WS2812 on GPIO8** should light up (blinks at least once / stays dimly lit). With the default Arduino sketch, the WS2812 cycles through colors.
- Optionally: connect the ESP32 USB-C to a computer, open Serial Monitor (115200 baud) — observe boot logs.

> 💡 ESP32-C6-DevKitM-1 has an auto-select power source: you can simultaneously keep `VIN = 5 V` from the Buck and connect the ESP32 USB-C to a computer for flashing/logs. This is a convenient debug mode.

If ESP32 does not boot:
- Re-check VIN/GND polarity on the pin headers.
- Measure VIN on the board: should be 5 V.
- Remove the Buck from the breadboard, measure its output directly — if 5 V is present there, the problem is on the breadboard (bad contact).

**Unplug USB-C before the next phase.**

---

## Phase 4 — Connect GPIO ↔ relay (still no pump)

> The most delicate step. Here we add the pull-down so the relay is guaranteed to be off during ESP32 boot/reset.

### 4.1 ⚠ Pull-down 4.7 kΩ on IN1

Insert a 4.7 kΩ resistor (from kit #10) between:
- one end → the `IN1` line (where GPIO18 will be connected);
- other end → `GND` rail.

**Why:** during ESP32 reset, GPIO18 is in `High-Z input` (floating) for several hundred milliseconds. Many relay modules have an internal pull-up on the IN pin to 5 V through the optocoupler — without an external pull-down the relay will accidentally close on every reset/power-up. With a 4.7 kΩ resistor to GND the line is guaranteed to be pulled to ~0 V and the relay stays off.

> Without a float switch (see SVG: info block on the right) this is the **only** protection against accidental pump activation during an ESP reboot. **Do not skip.**

(Optionally) An identical 4.7 kΩ pull-down on `IN2` — if you plan to ever use CH2.

### 4.2 GPIO ↔ IN

| From | To |
|---|---|
| ESP32 `GPIO18` (J3.9) | relay `IN1` |
| ESP32 `GPIO19` (J3.8) | relay `IN2` (opt.) |

### 4.3 Flash a relay blink test sketch

Connect the ESP32 USB-C to a computer. Upload (test main.cpp, not the final firmware):

```cpp
#include <Arduino.h>

constexpr uint8_t kRelayIn1Pin = 18;

void setup() {
  Serial.begin(115200);
  pinMode(kRelayIn1Pin, OUTPUT);
  digitalWrite(kRelayIn1Pin, LOW);  // explicit safe state before everything else
  Serial.println("Relay blink test");
}

void loop() {
  digitalWrite(kRelayIn1Pin, HIGH);
  Serial.println("ON");
  delay(1000);
  digitalWrite(kRelayIn1Pin, LOW);
  Serial.println("OFF");
  delay(1000);
}
```

Build and flash:
```bash
pio run -e coordinator -d firmware/coordinator -t upload
```

### ✅ Check #6 — relay clicks once per second

- A rhythmic click-click is audible at 1 s intervals.
- Channel 1 LED on the relay module blinks in sync.
- Serial Monitor shows `ON / OFF` in rhythm.

### ✅ Check #7 — pull-down works during reset

With PD-trigger power on:
- Press the **RST** button on ESP32 during the "OFF" phase of the sketch — the relay must not click accidentally.
- Press RST during the "ON" phase — the relay must turn off (GPIO18 is in high-Z during reset, pull-down holds it at 0 V) and **must not** return to HIGH until the next loop() iteration.

If the relay clicks "on its own" during reset — the pull-down is wired incorrectly (or its value is above 10 kΩ — use exactly 4.7 kΩ).

**Unplug USB-C from PD-trigger. The ESP32 USB-C connected to the computer can stay.**

---

## Phase 5 — Connect the pump

> Point of no return: from here on, the load is real.

### 5.1 ⚡ Confirm power is disconnected

USB-C from PD-trigger is unplugged? If not — unplug it.

### 5.2 Connect the pump

| From | To |
|---|---|
| Pump `+` (red wire) | relay `NO1` (screw terminal) |
| Pump `−` (black wire) | `GND` breadboard rail |

**Tighten the relay screw terminals firmly** — a loose contact causes arcing / heat.

### 5.3 Submerge the pump in a cup of water

A cup or jar ≥ 0.5 L. The pump must be **fully submerged** so the water covers the entire housing and the intake port.

Route the outlet hose into a sink / second cup.

### 5.4 Multimeter check

- `NO1` ↔ `GND` → 0 V (relay off, 0 V on the pump).
- Continuity pump `−` wire ↔ `GND` rail → beeps.

### 5.5 Apply PD-trigger power

Plug in USB-C.

### ✅ Check #8 — pump starts and stops

The sketch from step 4.3 continues cycling. Each second:
- During HIGH phase (LED1 on, click): **pump spins, water flow visible through the hose, BLDC noise audible**.
- During LOW phase: pump **stops immediately** (BLDC with built-in driver, minimal inertia).

Optionally verify current with an ammeter in series with the pump `+` wire:
- Peak at startup: ~2 A.
- Steady state: ~1–1.5 A.
- During LOW phase: 0 A.

If the peak exceeds 2.5 A — the Buck may go into protection. In our circuit this **should not happen** because the pump is powered directly from the PD-trigger (bypassing the Buck), but if wires were accidentally swapped — check.

### 5.6 ⚠ Safe reboot test

With the pump in the **off phase** of the sketch (LED1 off), press the **RST** button on ESP32. The pump **must not** twitch even for a moment. If it twitches — the pull-down is not working; do not proceed until it is fixed.

**Unplug USB-C, lift the pump out of the water, dry it off.**

---

## Phase 6 — Flash the real logic (RelayPump + IrrigationService)

> The test blink sketch is replaced by the full firmware. By this phase the hardware has proven itself; what follows is software only.

### 6.1 What the firmware must contain (per architecture [CLAUDE.md §1, §3](../../CLAUDE.md))

```
firmware/coordinator/lib/
├── infrastructure/drivers/RelayPump.{hpp,cpp}   ← new
├── application/IrrigationService.{hpp,cpp}      ← new
└── domain/ports/IPump.hpp                       ← already specified
```

`RelayPump` takes in its constructor:
- `gpio_num` (= 18)
- `max_runtime_ms` (= 20'000)
- a reference to `IClock`

Inside `start()` — `digitalWrite(gpio, HIGH)` + store `start_time`. In `tick()` (called from the main task) — check `clock.now() - start_time > max_runtime_ms` → force `stop()`.

### 6.2 ⚠ The key invariant — safe state in setup()

In `main.cpp::setup()` **as the very first line** after `Serial.begin`:
```cpp
pinMode(kRelayIn1Pin, OUTPUT);
digitalWrite(kRelayIn1Pin, LOW);
```
Before Wi-Fi init, MQTT init, or anything else.

### 6.3 ✅ Check #9 — 20 s watchdog

Send `pump.start()` via REST / Serial / MQTT and do **not** send stop.

After 20 seconds:
- The pump must **stop on its own**.
- Serial logs — message `IrrigationService: max runtime exceeded, force-stop`.

### 6.4 ✅ Check #10 — reset during pumping

Trigger `pump.start()`. After 5 seconds press **RST** on ESP32.

- The pump must stop **instantly** (ESP32 boots → pull-down holds IN1 LOW → relay OFF).
- After firmware loads: pump stays off (firmware sets LOW in setup() and does not start on its own until a command arrives).

### 6.5 Host tests

```bash
pio test -e coordinator-native -d firmware/coordinator
```

Unit tests for `IrrigationService` with `FakeClock` / `FakePump` must pass (see [CLAUDE.md §7](../../CLAUDE.md)) — runtime limit, double-start protection, etc.

---

## Don't-do checklist

- ❌ Never run the pump dry — not even for 2 seconds to test.
- ❌ Never skip the 4.7 kΩ pull-down on `IN1`.
- ❌ Never apply voltage > 3.3 V to `IN1`.
- ❌ Never share ground through a chassis / metal workbench — only a direct GND rail conductor.
- ❌ Never flash ESP32 without a safe state in `setup()` (`digitalWrite(GPIO18, LOW)` as the first line).
- ❌ Never use the ESP32 USB-C port as the power source for the whole system — it is 5 V and insufficient for the pump. For flashing/Serial only.
- ❌ Never leave the assembled system powered unattended until Check #9 (20 s watchdog) has passed.
- ❌ Never set `kPumpMaxRuntimeMs` above 60 seconds without a float switch — this is the hard cap against flooding.

---

## Troubleshooting

| Symptom | Where to look | Fix |
|---|---|---|
| PD-trigger outputs 5 V instead of 12 V | S1 lock not soldered / adapter does not support 12 V / charging-only cable | Phase 0.1; try a different adapter/cable |
| Buck outputs 0 V | IN/OUT swapped or input is not receiving 12 V | Phase 1.5 |
| ESP32 does not boot | VIN/GND polarity reversed / GND wire broken | Phase 3.3 — check VIN with multimeter |
| Relay does not click from GPIO | Jumper in LOW-active / IN threshold > 3.3 V | Phase 0.2 → HIGH-active; if that does not help — BC547 buffer ([COMPONENTS.md:204](../../COMPONENTS.md#L204)) |
| Relay clicks on ESP reset | Pull-down missing or > 10 kΩ | Phase 4.1 — install exactly **4.7 kΩ** |
| Pump hums but does not pump | `+/−` polarity reversed / pump sucked an air bubble | Phase 0.3 — re-check polarity; submerge deeper |
| Pump does not stop after 20 s | `kPumpMaxRuntimeMs` not triggered / `tick()` not being called | Phase 6.3 — verify the main task calls `pump.tick()` regularly |
| ESP32 loses power under load | Buck in overcurrent protection / VIN wire too thin | Confirm pump bypasses the Buck (Phase 5.2) |

---

## Out of scope

- **Float switch / hardware interlock** — for a production build. Without it the 20 s watchdog is the only safeguard; that is insufficient for long-term operation.
- **IP54 enclosure** — the system is still on breadboard. Moving to a permanent mount is a separate task.
- **Sensor-node** — covered in a separate document ([sensor-node-wiring.svg](sensor-node-wiring.svg)). It connects to the coordinator over Zigbee only — no physical wires.
- **Full firmware** — this document covers assembly and bring-up only. Firmware architecture is in [CLAUDE.md](../../CLAUDE.md); `IrrigationService` implementation details are in `firmware/coordinator/lib/application/`.
