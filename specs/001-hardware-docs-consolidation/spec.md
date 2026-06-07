# Feature Specification: Hardware Documentation Consolidation

**Feature Branch**: `001-hardware-docs-consolidation`

**Created**: 2026-06-05

**Status**: Draft

**Input**: User description: "Hardware information is scattered across multiple places and causes AI hallucinations. We need a strict folder structure and a clear understanding of what connects to what, plus sensor and chip characteristics."

## Problem Statement

Hardware facts for the Greenhouse ESP32-C6 project are duplicated across at least
four independent locations, and some of those copies **contradict each other**.
This is the root cause of AI agents (and humans) hallucinating pin numbers,
addresses and calibration values.

Current sources of the same facts:

| Source | What it holds | Problem |
|---|---|---|
| `COMPONENTS.md` (repo root) | 21-item BOM + per-part datasheet facts + a GPIO map | Authoritative for BOM, but also re-states pins/addresses that live elsewhere |
| `firmware/CLAUDE.md`, `firmware/coordinator/CLAUDE.md`, `firmware/sensor-node/CLAUDE.md` | Per-board pinouts, strapping-pin notes | A 4th copy of the pin tables; aimed at the AI agent, not a hardware reference |
| `docs/hardware/**` (uncommitted) | `boards/`, `sensors/`, `datasheets/`, `photos/`, wiring SVGs | The intended home, but partial and not yet the single truth |
| firmware config headers (`AppConfig.hpp`, `CoordinatorConfig.hpp`, `SensorNodeConfig.hpp`) | The pins/addresses/calibration the firmware **actually uses** | The ground truth for runtime — but docs drift from it |

### Confirmed contradictions (must be resolved, not just documented)

1. **Chirp soil calibration drift.** `AppConfig.hpp` ships defaults `raw_dry=300 /
   raw_wet=700`, while `COMPONENTS.md` and `sensor-node/CLAUDE.md` (and project
   memory) state the **measured** unit values `raw_dry=249 / raw_wet=489`. A
   reader cannot tell which is correct.
2. **Float switch reality gap.** `COMPONENTS.md` lists the float switch as
   "mandatory for production" and `coordinator/CLAUDE.md` reserves `GPIO14` for
   it, but `coordinator/src/main.cpp` wires `FakeFloatSwitchAlwaysOk`. The doc
   implies hardware that is not installed.
3. **AM2315C supply-voltage ambiguity.** Datasheet range "2.2–5.5 V" is stated
   next to "power strictly at 3.3 V"; the *why* (VCC-referenced I²C levels would put
   5 V on a 3.3 V-max GPIO) is buried and easy to misread.

### Missing artifact

There is **no single connection matrix** ("pin → net → what it connects to").
The wiring is reconstructable only by cross-reading `coordinator-assembly.md`,
two SVGs, three CLAUDE.md files and the BOM — exactly the fan-out that produces
hallucinations.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Single authoritative hardware reference (Priority: P1)

A developer (human or AI agent) needs to know "what GPIO drives the pump?" or
"what is the Chirp I²C address and calibration?" and must get **one** answer,
fast, without cross-referencing four files or guessing between conflicting copies.

**Why this priority**: This is the whole point of the feature. Without a single
source of truth, every other improvement is cosmetic. Eliminating contradictions
directly removes the hallucination triggers.

**Independent Test**: Pick any pin / I²C address / calibration value. Search the
repo. Confirm exactly one canonical statement exists under `docs/hardware/`, and
every other mention (CLAUDE.md, firmware comment) either points to it or is gone.
Confirm the canonical value matches what the firmware actually uses.

**Acceptance Scenarios**:

1. **Given** the consolidated docs, **When** a reader looks up the pump relay
   pin, **Then** `docs/hardware/` states `GPIO18` in exactly one place and the
   firmware constant `kPumpRelayGpio` matches it (18).
2. **Given** the Chirp calibration, **When** a reader looks it up, **Then** doc
   and `AppConfig.hpp` agree on a single pair of `raw_dry/raw_wet` values, with a
   note distinguishing "shipped firmware default" from "measured per-unit value".
3. **Given** any board pinout, **When** the reader compares the doc table to the
   firmware config header, **Then** every pin/address/frequency is identical.

### User Story 2 - Connection matrix ("what connects to what") (Priority: P1)

A builder assembling or debugging the rig needs an at-a-glance map of every
electrical connection: each MCU pin, the net it sits on, the component on the
other end, and any inline part (pull-up, pull-down, level shifter).

**Why this priority**: The user explicitly asked for "a clear understanding of what connects to what".
A connection matrix is the artifact that answers it and is currently
absent.

**Independent Test**: Open the connection matrix for one board and physically
trace three connections on the breadboard without opening any other file.

**Acceptance Scenarios**:

1. **Given** the coordinator connection matrix, **When** the reader looks up
   `GPIO6`, **Then** they see `I²C SDA → Chirp + AM2315C, 4.7 kΩ pull-up → 3V3,
   header J1.10`.
2. **Given** the matrix, **When** the reader looks up the relay IN1 net, **Then**
   they see `GPIO18 → relay CH1 IN, 4.7 kΩ pull-down → GND (safe-on-reset)`.
3. **Given** the sensor-node matrix, **When** the reader looks up `GPIO4`, **Then**
   they see `p-MOSFET sensor-power gate, external 100 kΩ pull-up → VBAT`.

### User Story 3 - Per-component characteristics that match reality (Priority: P2)

A reader needs the electrical characteristics (voltage, I²C address, pull-up
requirement, current, calibration, gotchas) for each chip/sensor/actuator, with
the production-vs-stub status made explicit.

**Why this priority**: The characteristics largely exist already; the value here
is correctness and status honesty (float switch, USB-CDC flags), which is a
refinement on top of P1.

**Independent Test**: For each active component, the per-component README states
voltage, address (if any), pull-up requirement, and current firmware status
(installed / stubbed / planned), and none contradicts the connection matrix.

**Acceptance Scenarios**:

1. **Given** the float-switch documentation, **When** read, **Then** it clearly
   states the hardware is **not yet installed** and the firmware currently uses a
   fake always-OK switch.
2. **Given** the AM2315C page, **When** read, **Then** the "2.2–5.5 V vs strictly
   3.3 V" point is stated once with the reason (VCC-referenced I²C levels).

### Edge Cases

- **A value changes in firmware** (e.g. someone re-calibrates Chirp): the process
  must define where it is updated first and how the doc/firmware stay in sync.
- **A new sensor or board is added**: the structure must have an obvious slot for
  it without restructuring.
- **A pin is reassigned**: the connection matrix and the per-board pinout must not
  be able to disagree (ideally one is derived from / references the other).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: `docs/hardware/` MUST be the single canonical home for all hardware
  facts (pins, I²C addresses, bus frequencies, calibration, electrical specs,
  wiring). Decision: docs canonical, firmware references docs.
- **FR-002**: Every hardware fact MUST appear in exactly one canonical location;
  all other mentions MUST either link to it or be removed.
- **FR-003**: The structure MUST provide a **connection matrix** per board mapping
  `pin → net → connected component(s) → inline parts → header label`.
- **FR-004**: The structure MUST provide a **per-board** reference (chip specs,
  full pinout, strapping pins, gotchas) for the DevKitM-1 (coordinator) and the
  SuperMini (sensor-node).
- **FR-005**: The structure MUST provide a **per-component** reference for each
  sensor and actuator (AM2315C, Chirp soil, relay+pump, transistors, battery
  chain) with electrical specs and firmware status.
- **FR-006**: The three confirmed contradictions MUST be **resolved** (single
  agreed value/status), not merely annotated:
  - Chirp calibration: one canonical pair, with "shipped default" vs "measured
    per unit" explicitly separated; firmware default reconciled or documented as
    intentionally generic.
  - Float switch: documented as not-installed / firmware-stubbed.
  - AM2315C supply voltage: stated once with rationale.
- **FR-007**: Each firmware config constant that encodes a hardware fact
  (`AppConfig.hpp`, `CoordinatorConfig.hpp`, `SensorNodeConfig.hpp`,
  `Wire.begin(...)`) MUST carry a comment pointing to the canonical doc anchor.
- **FR-008**: The three `firmware/**/CLAUDE.md` pin tables MUST stop being a
  parallel source of truth — either trimmed to a pointer into `docs/hardware/` or
  explicitly marked as a non-authoritative agent summary that mirrors the docs.
- **FR-009**: `COMPONENTS.md` MUST remain the BOM/purchasing source but MUST
  delegate pin/address/calibration facts to `docs/hardware/` via links rather than
  restating them.
- **FR-010**: The structure MUST include an index/README that lets a reader reach
  any board, sensor, actuator, or the connection matrix in one hop.
- **FR-011**: A short **maintenance/sync rule** MUST be documented: when a
  hardware fact changes, update the canonical doc first, then the referencing
  firmware comment/constant.
- **FR-012**: Existing assets (wiring SVGs, photos, datasheets, assembly guide)
  MUST be retained and linked from the new structure, not orphaned.

### Key Entities

- **Board**: an MCU dev board (DevKitM-1 coordinator, SuperMini sensor-node) —
  chip, power, full pinout, strapping pins, gotchas.
- **Component**: a sensor or actuator (AM2315C, Chirp, relay, pump, transistors,
  battery chain) — electrical specs, address, pull-up needs, firmware status.
- **Net / Connection**: an electrical link — pin, net name, endpoints, inline
  parts (pull-up/down, level shifter), header label.
- **Canonical fact**: a single (key → value) hardware datum (e.g. `pump_relay_gpio
  = 18`) that is stated once and referenced everywhere else.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: For every pin, I²C address, bus frequency and calibration value
  there is **exactly one** canonical statement in `docs/hardware/` (zero
  duplicated authoritative copies).
- **SC-002**: **Zero** contradictions remain between `docs/hardware/`, the
  `CLAUDE.md` files, `COMPONENTS.md`, and the firmware config headers — verifiable
  by a diff/grep of the key values.
- **SC-003**: A reader can answer "what connects to GPIO X on board Y?" by opening
  **one** file (the connection matrix), with no cross-referencing.
- **SC-004**: 100% of hardware-encoding firmware constants carry a comment
  pointing to their canonical doc anchor.
- **SC-005**: An AI agent asked "what is the pump pin / Chirp address /
  calibration / float-switch status?" returns the canonical value/status with a
  single-file citation and no contradicting alternative.

## Assumptions

- Decision (from clarification): `docs/hardware/` is canonical; firmware
  references it by comment. Sync is manual, governed by the documented rule
  (FR-011); no code-generation pipeline is built in this iteration.
- Scope (from clarification): structure **plus** resolution of the three
  contradictions. Hardware changes (actually installing the float switch,
  re-calibrating Chirp) are out of scope — only the documentation and firmware
  comments/defaults are reconciled.
- The existing partial `docs/hardware/` tree (boards/, sensors/, datasheets/,
  photos/, SVGs) is the foundation to build on, not to discard.
- No physical rewiring is implied; the docs describe the rig as currently built
  plus clearly-flagged not-yet-installed items.
- Resolving the Chirp default in firmware (300/700 → 249/489 or a documented
  generic default) is a one-line config change reviewed with the maintainer, not a
  behavioural redesign.
