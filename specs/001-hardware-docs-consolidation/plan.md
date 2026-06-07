# Implementation Plan: Hardware Documentation Consolidation

**Branch**: `001-hardware-docs-consolidation` | **Date**: 2026-06-05 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/001-hardware-docs-consolidation/spec.md`

## Summary

Make `docs/hardware/` the single canonical source of truth for every hardware
fact (pins, I²C addresses, bus frequencies, calibration, electrical specs,
wiring), eliminate the duplicate copies in `COMPONENTS.md` and the three
`CLAUDE.md` files, add the missing **connection matrix** per board, and resolve
the three confirmed contradictions (Chirp calibration, float-switch status,
AM2315C supply voltage). Firmware config constants gain comments pointing at the
canonical doc anchors. No physical rewiring, no code-gen pipeline — manual sync
governed by one documented rule.

The technical heart is a **canonical-values reference table**
(`docs/hardware/reference/canonical-values.md`): one row per hardware datum, each
stated exactly once. Every other document (board pinouts, sensor pages,
connection matrices) and every firmware comment links *into* that table instead of
restating the number.

## Technical Context

**Language/Version**: Markdown documentation (GitHub-flavored) + minor C++17 edits
to firmware config headers (comment-only, plus one calibration-default decision).

**Primary Dependencies**: None new. Existing assets reused: `COMPONENTS.md`,
`docs/hardware/**` (partial), wiring SVGs, BC547/BC557 datasheets, photos,
`coordinator-assembly.md`.

**Storage**: Files in the repo. No database, no generated artifacts.

**Testing**: Verification is grep/diff-based consistency checks (every canonical
value appears once; doc value == firmware constant). Optional: a small
`scripts/check-hw-consistency.sh` that greps the canonical table vs the config
headers and fails on drift (stretch, not required for P1).

**Target Platform**: Repository docs; firmware targets ESP32-C6 (DevKitM-1
coordinator, SuperMini sensor-node) — unchanged by this feature except comments.

**Project Type**: Documentation restructure within an existing monorepo
(firmware + services + docs).

**Performance Goals**: N/A (docs). Success is measured by zero duplication / zero
contradiction (see spec Success Criteria).

**Constraints**:
- Docs canonical; firmware references docs (clarified decision).
- Do not orphan existing assets (FR-012).
- Embedded code rules still apply to any firmware edit (fixed-width types,
  `constexpr`, no behavioral change) per `~/Work/CLAUDE.md` and project CLAUDE.md.

**Scale/Scope**: 2 boards, 2 I²C sensors, 1 relay+pump actuator, 2 transistors,
1 float switch (stubbed), 2 power chains (coordinator mains-ish, sensor-node
battery). ~12 canonical facts that currently live in 2–4 places each.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

The project constitution (`.specify/memory/constitution.md`) is still the unfilled
template — no ratified principles to gate against. Applying the **repository's**
governing rules instead:

- **Clean Architecture (project CLAUDE.md §1)** — N/A for docs; firmware edits are
  comment-only and touch the composition-root config headers, not domain logic.
  PASS.
- **Single source of truth** — this feature *establishes* it. PASS (by design).
- **No behavioral firmware change** — the only non-comment firmware edit is the
  Chirp calibration default, which is a config value reconciliation reviewed with
  the maintainer; gated behind an explicit decision in Phase 2. PASS with note.
- **Pre-commit checklist (project CLAUDE.md §10)** — docs-only changes skip the
  `pio` build/test gates; if the calibration default is changed,
  `pio test -e coordinator-native` and `-e sensor-node-native` MUST be run. NOTED.

No violations requiring Complexity Tracking.

## Project Structure

### Documentation (this feature)

```text
specs/001-hardware-docs-consolidation/
├── spec.md              # Feature spec (done)
├── plan.md              # This file
├── research.md          # Phase 0: inventory of every scattered fact + contradictions
├── data-model.md        # Phase 1: the canonical-fact schema + target doc tree
├── quickstart.md        # Phase 1: "how to find / how to update a hardware fact"
├── contracts/
│   └── canonical-values.schema.md   # the columns each canonical row must have
└── tasks.md             # Phase 2 output (/speckit-tasks — NOT created here)
```

### Target documentation tree (the deliverable)

```text
docs/hardware/
├── README.md                         # index: one hop to any board/sensor/actuator/matrix/values
├── reference/
│   └── canonical-values.md           # ★ SINGLE SOURCE OF TRUTH: pins, I²C addrs, freqs, calibration
├── connections/
│   ├── coordinator-matrix.md         # pin → net → component(s) → inline part → header label
│   └── sensor-node-matrix.md
├── boards/
│   ├── esp32-c6-devkitm-1/README.md  # chip, power, full pinout, strapping pins, gotchas
│   └── esp32-c6-supermini/README.md
├── sensors/
│   ├── am2315c/README.md             # 0x38, 3.3V-only rationale, no pull-up → 4.7kΩ
│   └── chirp-soil-moisture/README.md # 0x20, calibration (default vs measured), warmup
├── actuators/
│   ├── relay-pump/README.md          # GPIO18 CH1, active-high, pull-down, 20s watchdog
│   ├── transistors-bc547-bc557/README.md
│   └── float-switch/README.md        # STATUS: not installed; firmware = FakeFloatSwitchAlwaysOk
├── power/
│   ├── coordinator-power.md          # USB-PD 12V → pump (direct) + buck 5V → ESP32 + relay coils
│   └── sensor-node-battery.md        # 18650 VTC6, LTH7R charger, 1S protection, soft-cutoff
├── assembly/
│   └── coordinator-assembly.md       # moved from docs/hardware/coordinator-assembly.md
├── datasheets/                       # existing — BC547.pdf, BC557.pdf
├── photos/                           # existing
└── *.svg                             # existing wiring/breadboard diagrams, linked from matrices
```

### Firmware touch-points (comment + one value)

```text
firmware/shared/application/src/AppConfig.hpp          # comments → canonical-values.md anchors; Chirp default decision
firmware/coordinator/lib/application/src/CoordinatorConfig.hpp   # comment → relay-pump anchor
firmware/sensor-node/lib/application/src/SensorNodeConfig.hpp    # comments → board/sensor anchors
firmware/sensor-node/src/main.cpp                      # Wire.begin(6,7,...) comment → i2c-bus anchor
firmware/CLAUDE.md, firmware/coordinator/CLAUDE.md, firmware/sensor-node/CLAUDE.md
                                                        # pin tables → trimmed to pointers / marked non-authoritative
COMPONENTS.md                                          # pin/addr/calibration facts → links into docs/hardware
```

**Structure Decision**: A flat-by-concern layout under `docs/hardware/` with a
dedicated `reference/canonical-values.md` as the one authoritative table, plus
`connections/` for the new matrices. This keeps the already-built `boards/`,
`sensors/`, `datasheets/`, `photos/` and the assembly guide in place (FR-012) and
gives new boards/sensors an obvious slot (spec Edge Case) without restructuring.

## Phased Approach

> Detailed, ordered tasks are generated by `/speckit-tasks` into `tasks.md`. The
> phases below define the sequence and exit criteria.

### Phase 0 — Inventory & contradiction lock (research.md)
- Catalogue every hardware fact and its current location(s) (the Explore inventory
  already done seeds this).
- Pin down the canonical value for each of the 3 contradictions, with the
  maintainer's call on the Chirp firmware default.
- **Exit**: `research.md` lists each fact, its locations, and the agreed canonical
  value/status. No open `NEEDS CLARIFICATION`.

### Phase 1 — Schema & skeleton (data-model.md, contracts/, quickstart.md)
- Define the canonical-fact row schema (key, value, unit, board, firmware
  constant, doc anchor) → `contracts/canonical-values.schema.md`.
- Create the target `docs/hardware/` tree with stub headers + the empty canonical
  table.
- Write `quickstart.md`: "how to look up a fact" and "how to change a fact".
- **Exit**: tree exists; every file has a heading + anchor; canonical table has all
  rows keyed (values may still be migrating).

### Phase 2 — Migrate facts into canonical table + matrices (P1)
- Fill `reference/canonical-values.md` from the verified inventory (one row each).
- Author `connections/coordinator-matrix.md` and `sensor-node-matrix.md`.
- Resolve the 3 contradictions in their canonical home; if the Chirp firmware
  default changes, edit `AppConfig.hpp` and run native tests.
- **Exit**: SC-001, SC-002, SC-003 hold for the canonical table + matrices.

### Phase 3 — De-duplicate the satellites (P1)
- Replace pin/addr/calibration restatements in the 3 `CLAUDE.md` files and
  `COMPONENTS.md` with links into `docs/hardware/` (FR-008, FR-009).
- Add doc-anchor comments to firmware config constants (FR-007, SC-004).
- **Exit**: SC-004 holds; grep finds no second authoritative copy of any value.

### Phase 4 — Per-component depth + index + sync rule (P2)
- Flesh out board/sensor/actuator/power READMEs to match-reality (float-switch
  status, USB-CDC flags, AM2315C rationale).
- Write `docs/hardware/README.md` index (FR-010) and the maintenance/sync rule
  (FR-011); link existing SVGs/photos/datasheets (FR-012).
- **Exit**: all FR met; SC-005 spot-checked by asking an agent the 4 canonical
  questions.

### Phase 5 — Verify & integrate
- Run the consistency check (grep canonical table vs firmware constants).
- If firmware changed: `pio test -e coordinator-native` and `-e sensor-node-native`.
- Open PR for the docs branch.

## Complexity Tracking

> No constitution violations to justify. Table intentionally empty.

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| — | — | — |
