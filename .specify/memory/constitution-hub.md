# Greenhouse ESP32 — Hub Backend Constitution

> Governs: `services/hub/`
> Python 3.12 · FastAPI 0.115 · SQLAlchemy 2.0 async · PostgreSQL 16 + TimescaleDB
>
> Parent: [`.specify/memory/constitution.md`](constitution.md)

## Core Principles

### I. Layered Architecture — Thin Routes, Logic in Repositories

Three layers; dependencies flow downward only:

```
routes/          FastAPI endpoints — thin, no business logic
    │
repositories/    All DB access; returns ORM objects, dataclasses, or None
    │
models.py        SQLAlchemy Mapped[...] ORM models
schemas/         Pydantic v2 wire contracts (In / Out / Patch)
db.py            Async engine + session factory
config.py        Pydantic Settings (env-driven, extra="forbid")
```

**MUST**: routes call repository functions with `AsyncSession` + Pydantic payload.
Repositories return typed results — never raw dicts or untyped rows.
Routes serialize via `<Name>Out.model_validate(obj, from_attributes=True)`.

**MUST NOT**: put SQL (`select(...)`, `text(...)`) inside route handlers. Business
logic (validation, branching, authorization checks beyond `Depends`) belongs in
repositories or a service layer — not in the route body.

### II. Device Profile Catalog — Migration-Only Authority

The `device_profiles` table is the hub's knowledge of what devices exist. It is
populated **exclusively through Alembic migrations** — never via HTTP endpoints.

To support a new device type:
1. Write a new migration with `INSERT INTO device_profiles ...`
2. Apply with `make migrate`
3. Firmware must send the matching `profile_id` string in the pairing claim body

**MUST NOT**: expose any HTTP endpoint that creates or modifies `device_profiles`.
This is non-negotiable — it is what makes the hub "know about devices" rather than
accept anonymous streams.

### III. Migration Integrity — Raw SQL, Append-Only

All Alembic migrations use `op.execute("raw SQL")` — not ORM `autogenerate`. Reason:
Timescale-specific SQL (`create_hypertable`, `SELECT FOR UPDATE`, `NOT NULL` backfills
via three-step `ADD COLUMN → UPDATE → SET NOT NULL`) cannot be expressed cleanly
through SQLAlchemy MetaData. `alembic/env.py` sets `target_metadata = None` deliberately.

**MUST NOT**: modify an existing migration file after it has been applied anywhere
(local, staging, production). Changes go into a new `000N+1_fix_xxx.py`.

**MUST NOT**: use `alembic autogenerate` — it would produce ORM-style diffs that
conflict with the raw-SQL convention.

### IV. Async-First — No Sync SQLAlchemy in Application Code

All database access uses SQLAlchemy async (`AsyncSession`, `async_sessionmaker`).
No sync `Session` is allowed in `routes/`, `repositories/`, or `auth*.py`.

For operations the ORM cannot express cleanly, use `text(...)`:
- `SELECT … FOR UPDATE` (claim_atomic race-safety)
- `tuple_(a, b).in_(pairs)` (batch validation — prefer ORM here)
- Hypertable-specific raw SQL

If an external library is sync-only, wrap with `asyncio.to_thread(...)`.

**MUST NOT**: import `sqlalchemy.orm.Session` (sync) in any application module.

### V. Secrets — Issue Once, Hash Forever

`api_key` and `admin_token` are issued exactly once and never returned again:

- On creation: plaintext returned in the HTTP response body, then discarded server-side.
- Stored: SHA-256 hash in `device_credentials.api_key_hash` /
  `admin_tokens.token_hash`.
- Lost key recovery: revoke the credential and issue a new one.
  `app/tools/mint_admin_token` (CLI, outside HTTP) is the emergency path.

**MUST NOT**: log `api_key`, `admin_token`, Wi-Fi password, or MAC address in
plaintext to stdout/stderr/logger, even at DEBUG level. Use `token_hash[:8]`
or `device_id` (public UUID) for diagnostics.

**MUST NOT**: return a stored token from any `GET` endpoint.

### VI. Atomic Pairing Claim — SELECT FOR UPDATE

`claim_atomic` in `repositories/pairing.py` uses `SELECT … FOR UPDATE` to serialize
concurrent claim attempts on the same 6-digit code. Two parallel claims with the same
code: one wins, one receives `WindowNotAvailable` (410).

**MUST NOT** replace this with `INSERT … ON CONFLICT DO NOTHING` — ON CONFLICT does
not lock existing rows on the read step and would allow both parallel claims to pass
the "not-yet-consumed" check simultaneously.

---

## Data Model Invariants

| Rule | Rationale |
|---|---|
| `sensors.last_value` is a hot cache, not source of truth | Source of truth = `readings` hypertable; cache is reconstructable |
| `readings` + `events` are hypertables; no FK to `devices` | TimescaleDB hypertables cannot have FKs to non-hypertables reliably; hard delete goes via raw SQL |
| `device_profiles` rows are immutable after insert | Same as migration integrity — changed via new migration only |
| All `In` / `Patch` schemas use `extra="forbid"` | Unknown fields from clients → 422 early, not silent discard |

---

## Development Commands

```bash
make dev      # uvicorn --reload (needs external Postgres + .env with DB_URL)
make up       # docker compose up -d --build
make migrate  # alembic upgrade head (inside hub container)
make test     # pytest -v (requires Docker daemon for testcontainers)
make smoke    # full e2e: up → migrate → mint token → pair → ingest → print results
```

Test strategy: `pytest` + `httpx` ASGI + `testcontainers-postgres` (one Timescale
container per test session). Tests use unique `device_id` / names — never `TRUNCATE`
between tests.

---

## Governance

Defers to the Master Constitution for amendment procedure and version policy.
Sub-constitution version is independent; bump it when hub-specific rules change.
Reference: [`.specify/memory/constitution.md`](constitution.md)

**Version**: 1.0.0 | **Ratified**: 2026-06-05 | **Last Amended**: 2026-06-07
