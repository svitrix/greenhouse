<!--
SYNC IMPACT REPORT
==================
Version change:    1.0.0 → 1.1.0
Bump rationale:    MINOR — master refactored into cross-cutting index; firmware-specific
                   principles extracted; hub and dashboard sub-constitutions added.

Modified principles:
  I.  Clean Architecture            → moved to constitution-firmware.md
  II. Memory-Safe Embedded Runtime  → moved to constitution-firmware.md
  III.Test-Before-Ship              → moved to constitution-firmware.md
  IV. Wire-Format and API Stability → stays here (cross-cutting)
  V.  Safety-First Actuator Control → moved to constitution-firmware.md

Added:
  I.  System Topology & Contracts (cross-cutting)
  II. Wire-Format and API Stability (promoted to master)
  III.Security Baseline (cross-cutting)
  Sub-constitutions index section
  constitution-firmware.md
  constitution-hub.md
  constitution-dashboard.md

Templates requiring updates:
  ✅ .specify/templates/* — no change needed

Deferred TODOs: none
-->

# Greenhouse ESP32 — Master Constitution

> Sub-domain constitutions:
> - **Firmware** (C++ embedded): [`.specify/memory/constitution-firmware.md`](constitution-firmware.md)
> - **Hub backend** (Python/FastAPI): [`.specify/memory/constitution-hub.md`](constitution-hub.md)
> - **Dashboard** (React/TS): [`.specify/memory/constitution-dashboard.md`](constitution-dashboard.md)
>
> This document covers only cross-cutting concerns that span all three domains.
> When a rule exists here AND in a sub-constitution, this document wins.

## Core Principles

### I. System Topology — Single Source of Truth per Layer

Each layer owns exactly one concern. Data flows in one direction; no layer polls or
calls "upward":

```
Sensor-node (Zigbee end-device)
    │ ZCL attribute reports
    ▼
Coordinator firmware (REST v2 · MQTT · Preact SPA)
    │ HTTPS batch (every 900 s)          │ MQTT (per-node, on-change)
    ▼                                    ▼
Hub backend (FastAPI · Timescale)    MQTT broker (Home Assistant)
    │ REST API
    ▼
Dashboard (React admin UI)
```

**MUST NOT** introduce a path that bypasses this topology (e.g. sensor-node talking
to hub directly, or dashboard writing to the coordinator database).

### II. Wire-Format and API Stability

**Zigbee**: cluster IDs, attribute IDs, endpoint numbers MUST NOT be renumbered after
any sensor-node has joined. The ZCL custom soil cluster is `0x0408`; attribute mapping
is frozen in `ChannelAttrTable.hpp`.

**REST**: version increments are atomic cutovers — no dual-API maintenance shims. v(N-1)
routes are deleted in one commit block the moment v(N) lands. Hub endpoint changes that
break the coordinator ingest contract MUST be coordinated with a firmware release.

**MQTT**: topic scheme changes MUST be accompanied by an NVS migration flag on the
coordinator to purge stale retained topics on first boot after upgrade.

**Ingest wire contract** (`POST /ingest` body shape) is shared between coordinator
firmware and hub. Changes require simultaneous update in both, tagged with the same
version string.

### III. Security Baseline

Applies to all domains equally:

- Secrets (`api_key`, `admin_token`, Wi-Fi password, MQTT password) MUST NOT be
  committed to git, logged to any output, or returned more than once via HTTP.
- `secrets.h` / `.env` files are in `.gitignore`; only `*.example` variants are
  tracked.
- Tokens are stored as SHA-256 hashes on the server side; the plaintext is returned
  exactly once at creation and never again.
- Debug builds MUST NOT print passwords, even to serial. `token_hash[:8]` or
  `device_id` (public) are acceptable for diagnostics.

---

## Sub-Constitution Scope

| Domain | File | Governs |
|---|---|---|
| Embedded firmware | `constitution-firmware.md` | Coordinator + sensor-node C++17, clean architecture, memory safety, RTOS, Zigbee, actuator control |
| Hub backend | `constitution-hub.md` | FastAPI service, SQLAlchemy 2.0, Alembic, pairing flow, device profiles, auth |
| Admin dashboard | `constitution-dashboard.md` | React 18, shadcn/ui, TanStack Query, Zod, design language |

Each sub-constitution is authoritative within its domain and adds domain-specific rules
on top of the three cross-cutting principles above.

---

## Governance

This master constitution supersedes all sub-constitutions, `CLAUDE.md` files, and
inline comments across all domains.

**Amendment procedure:**
1. Document the change with explicit rationale.
2. Bump `Version` (semver): MAJOR = principle removed/redefined; MINOR = new
   principle/section; PATCH = clarification/wording.
3. Run `/speckit-analyze` to surface alignment gaps.
4. Update `Last Amended` date (ISO 8601).
5. Commit: `docs: amend constitution to vX.Y.Z (<summary>)`

**Compliance**: Constitution violations are blocking — not mergeable with "fix later".

**Version**: 1.1.0 | **Ratified**: 2026-06-05 | **Last Amended**: 2026-06-07
