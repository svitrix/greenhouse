# CLAUDE.md — `services/hub/`

> Этот файл — локальный guide для AI / инженеров, работающих с backend-сервисом. Принципы и архитектура зафиксированы в [`../../.specify/memory/constitution-hub.md`](../../.specify/memory/constitution-hub.md). Системная топология — [`../../.specify/memory/constitution.md`](../../.specify/memory/constitution.md). Корневые правила — [`../../CLAUDE.md`](../../CLAUDE.md).

---

## [0] What this even is

Backend — **центральный hub** для всей экосистемы greenhouse-устройств, по принципу Philips Hue Bridge или Xiaomi Mi Home. Hub знает про **все типы устройств заранее** (через каталог `device_profiles`), управляет парком координаторов через REST + 6-digit pairing flow (Hue-стиль), и хранит сырые телеметрические данные в Timescale для будущей аналитики/ML.

**Текущая фаза (D-2 complete):**
- D-1 — ingestion pipeline (POST /ingest + dedup + Timescale hypertables) ✅
- D-1.5 — SQLAlchemy 2.0 ORM (raw SQL → Mapped) ✅
- D-2 — Hue-style pairing + admin REST + device profile catalog + hierarchy ✅

**Что вне backend (на будущих фазах):**
- D-3 — Admin Web UI (React/Next) поверх REST
- D-4 — Continuous aggregates, ML pipelines, anomaly detection
- D-5 — Reverse channel (backend → coordinator команды), rules engine, weather API
- Cloud deploy (Timescale Cloud / AWS RDS) — отдельный спек

**Target stack:**
- Python 3.14 (Dockerfile использует `python:3.14-slim`; та же версия в `services/bot/`)
- FastAPI 0.115, Uvicorn, SQLAlchemy 2.0 async (`asyncpg` driver)
- Pydantic v2 (Settings + schemas, везде `extra="forbid"`)
- Alembic для миграций (raw SQL — Timescale-specific `create_hypertable` не выражается через ORM)
- PostgreSQL 16 + TimescaleDB (`timescale/timescaledb:latest-pg16`)
- pytest + httpx ASGI + testcontainers-postgres

---

## [1] Current state — what works

```
┌──────────────────────────────────────────────────────────────────┐
│ /healthz                                  health                 │
│ /ingest                                   device bearer (D-1)    │
│ /api/pairing/{open,claim,windows}         admin / anon / admin   │
│ /api/locations                            admin CRUD             │
│ /api/plant_groups                         admin CRUD             │
│ /api/devices, /devices/{id}/{revoke,…}    admin                  │
│ /api/sensors[/{dev}/{ch}/{kind}]          admin list/get/patch   │
│ /api/admin/tokens                         admin self-management  │
│ /api/events                               admin list (D-5a)      │
│ /api/devices/{id}/commands                admin enqueue/list     │
│ /api/devices/{id}/commands/pending        device poll (D-5a)     │
│ /api/commands/{id}/ack                    device ack   (D-5a)    │
└──────────────────────────────────────────────────────────────────┘
```

The `device_commands` queue (migration `0006`) is the **reverse channel**:
admin (incl. the Telegram bot via its admin token) enqueues `pump_on` /
`pump_off`; the coordinator firmware will poll `/commands/pending` (which
atomically flips rows `pending → sent`) and `POST /commands/{id}/ack`. The
firmware side is not built yet — commands sit in `pending` until it is. The
Telegram bot is a **separate service** ([`../bot/`](../bot/)) that talks to
this API only — it holds an admin token, never a DB connection.

10 таблиц, 4 Alembic-миграции:
- `0001_initial`: devices, sensors, readings (hypertable), events (hypertable), device_credentials
- `0002_admin_and_pairing`: admin_tokens, pairing_windows
- `0003_hierarchy`: locations, plant_groups + ALTER devices/sensors (friendly_name, FK, last_value cache)
- `0004_device_profiles`: device_profiles + seed `gh-coordinator-v1` + ALTER devices ADD profile_id NOT NULL

Все 0002–0004 миграции писались **строго raw SQL** через `op.execute(...)` — причина в [§5.1](#51-почему-alembic-на-raw-sql).

---

## [2] Architecture — clean layers + Hue-style hub

### [2.1] Layered structure

Те же три слоя, что и в firmware-части проекта (см. root CLAUDE.md §1):

```
┌─────────────────────────────────────────────────────────────┐
│ routes/         FastAPI endpoints (thin, без бизнес-логики) │
├─────────────────────────────────────────────────────────────┤
│ repositories/   SQLAlchemy 2.0 ORM, контракт = функции,     │
│                 не модели. ON CONFLICT, SELECT FOR UPDATE   │
│                 живут здесь.                                 │
├─────────────────────────────────────────────────────────────┤
│ schemas/        Pydantic v2 — wire contracts. extra=forbid. │
│ models.py       SQLAlchemy ORM модели (Mapped[...]).        │
│ auth*.py        FastAPI Depends-инъекция (device / admin).  │
│ db.py           Async engine + session factory.             │
│ config.py       Pydantic Settings (env-driven).             │
└─────────────────────────────────────────────────────────────┘
```

**Контракт между слоями:**
- Route → Repository: вызов функции с `AsyncSession` + Pydantic-payload или примитивы.
- Repository → DB: ORM (`select(...)`, `pg_insert(...)`, `update(...)`) или `text(...)` для DB-специфичных конструкций (`SELECT FOR UPDATE`, `tuple_(...).in_()`).
- Repository → Route: возвращает Mapped-объект, dict, dataclass (`BatchInsertResult`, `OpenedWindow`, `ClaimResult`) или None.
- Route → клиент: `<Name>Out` Pydantic-модель с `from_attributes=True`.

### [2.2] Device profile catalog — Xiaomi property

Hub **знает про устройства**, а не «принимает анонимные потоки». Источник истины — `device_profiles` таблица, **заполняется только Alembic-миграциями**:

```sql
profile_id   PK             -- "gh-coordinator-v1"
sensor_specs JSONB NOT NULL -- [{channel_id, kind, unit, range_min, range_max, description}, ...]
```

Operator не может добавить новый profile через HTTP. Чтобы поддержать новое устройство:
1. Написать новую миграцию `0005_<name>.py` с `INSERT INTO device_profiles ...`
2. Применить (`make migrate`)
3. Прошивка координатора должна знать свой `profile_id` (compile-time constant)
4. На pairing'е координатор отправляет `profile_id` в `/api/pairing/claim` body

**Без этой записи в таблице claim упадёт 422** (`profile not registered with this hub`).

### [2.3] Three-tier auth

| Слой | Endpoint | Auth |
|---|---|---|
| Admin REST | всё под `/api/*` кроме `/ingest` и `/api/pairing/claim` | Bearer admin token (SHA-256 → `admin_tokens.token_hash`) |
| Device ingest | `POST /ingest` | Bearer device api_key (SHA-256 → `device_credentials.api_key_hash`) |
| Pairing claim | `POST /api/pairing/claim` | **Anonymous** — auth = сам 6-digit код в body |

Admin token bootstrap **только через CLI** (вне HTTP):
```
docker compose exec hub python -m app.tools.mint_admin_token --name "browser-laptop"
```
`mint_admin_token` живёт за пределами HTTP именно для emergency recovery — если все admin токены удалены, операцию всегда можно повторить через `docker compose exec`.

### [2.4] Hue-style pairing flow — atomic claim

`claim_atomic` в [`app/repositories/pairing.py`](app/repositories/pairing.py) — **load-bearing concurrency primitive**:

```python
async def claim_atomic(session, claim_code, device_id, ...):
    # 1) Lock the window row atomically
    locked = await session.execute(text(
        "SELECT code FROM pairing_windows "
        "WHERE code = :code AND expires_at > now() AND consumed_by IS NULL "
        "FOR UPDATE"))
    if locked.first() is None:
        raise WindowNotAvailable  # 410

    # 2) Reject if device already has credential
    if existing_cred:
        raise DeviceAlreadyRegistered  # 409

    # 3) Upsert device, INSERT credential, UPDATE window, INSERT sensors from profile
    ...
```

Два одновременных `claim_atomic` с одним и тем же кодом **сериализуются** через `SELECT FOR UPDATE`. Победителем становится один, второй получает `WindowNotAvailable`. Это тестируется в `test_repository_pairing::test_race_two_parallel_claims_exactly_one_wins` через `asyncio.gather` двух claim'ов.

**НЕ оптимизировать через `INSERT … ON CONFLICT`** — нужна гарантированная exclusive lock на чтении, чтобы два параллельных claim не прошли оба «проверку» до INSERT.

---

## [3] How data flows

### [3.1] Onboarding flow (новый координатор)

```
admin (CLI один раз) ──► docker compose exec hub
                          python -m app.tools.mint_admin_token --name "X"
                       ◄── prints 64-hex admin_token (хранит у себя)

admin (curl/UI/CLI) ──► POST /api/pairing/open  (Bearer admin)
                         body: {ttl_seconds: 300}
                      ◄── {code: "847291", expires_at: +5min}

operator физически вводит "847291" в captive portal координатора

coordinator (firmware) ──► STA-connect → POST /api/pairing/claim (anon)
                            body: {claim_code, device_id, mac, fw_version, profile_id}
                         ◄── {api_key: "...", device_id}
                         ── persists api_key в NVS, restart в operational mode

admin ──► GET /api/devices  → видит новый device + profile_id + 6 sensors
admin ──► PATCH /api/devices/{id} {friendly_name, location_id}
```

### [3.2] Telemetry ingestion (от D-1, валидация добавлена в D-2)

```
coordinator (~15 min flush) ──► POST /ingest  (Bearer device api_key)
                                 body: {device_id, fw_version, batch_id, readings: [...]}
                              ◄── 200 {accepted_readings, accepted_events, duplicates_skipped}

инside backend insert_batch():
  1. _upsert_device   — обновить last_seen_at / fw_version
  2. validation       — все (channel_id, kind) triplets ∈ sensors[device_id]?
                        если нет → UnknownSensorForDevice → 400
  3. bulk INSERT      — pg_insert(Reading).on_conflict_do_nothing(natural PK)
  4. denorm UPDATE    — sensors.last_value/last_value_at для max-ts per triplet
  5. INSERT events    — append-only
  6. COMMIT
```

### [3.3] Admin REST (после pairing)

| Сценарий | Endpoint |
|---|---|
| «Сколько у меня устройств online?» | `GET /api/devices` → каждое с `online` (last_seen_at > now()-30min) |
| «Какие сенсоры на координаторе X?» | `GET /api/sensors?device_id=X` |
| «Дать сенсору красивое имя» | `PATCH /api/sensors/{dev}/{ch}/{kind} {friendly_name: "Помидор #3"}` |
| «Привязать сенсор к группе растений» | `PATCH /api/sensors/.../{kind} {plant_group_id: <uuid>}` |
| «Записать калибровку Chirp» | `PATCH /api/sensors/.../soil_moist {calibration_json: {raw_dry: 249, raw_wet: 489}}` |
| «Отозвать ключ потерянного координатора» | `POST /api/devices/{id}/revoke` |
| «Удалить устройство + историю» | `DELETE /api/devices/{id}` (CASCADE на credentials + sensors + manual DELETE on readings/events) |

---

## [4] Build / test / smoke

```bash
# Локальная разработка
make dev          # uvicorn --reload (нужен внешний Postgres, .env с DB_URL)

# Docker Compose stack
make up           # docker compose up -d --build
make down         # tear down (сохраняет volume)
make logs         # hub logs
make psql         # interactive psql shell в timescaledb контейнере

# Миграции
make migrate      # alembic upgrade head (внутри hub контейнера)

# Тесты
make test         # pytest -v (требует Docker для testcontainers)

# Полный e2e smoke
make smoke        # up → migrate → mint admin → open pairing → claim →
                  # POST /ingest → распечатывает результаты
```

**Зависимости для тестов:**
- Docker daemon должен быть запущен (testcontainers-postgres поднимает Timescale image)
- `jq` для `make smoke` (parse JSON в bash)
- `.venv/` создаётся через `python3 -m venv .venv && .venv/bin/pip install -e ".[dev]"`

---

## [5] Decisions and conventions

### [5.1] Почему Alembic на raw SQL

Все 4 миграции пишутся через `op.execute("CREATE TABLE …")`, **не** через ORM `target_metadata`. Причины:

1. **Timescale-specific:** `SELECT create_hypertable('readings', 'ts')` не выражается через SQLAlchemy MetaData. Если бы часть схемы шла через ORM, а часть — через raw SQL, это были бы две независимые точки правды.
2. **Read-fast init:** `INSERT INTO device_profiles ... VALUES ($$...$$::jsonb)` — литералы в миграции читаются за один диск-seek, не нужны runtime hooks.
3. **NOT NULL backfill:** в 0004 сначала `ADD COLUMN profile_id (nullable)`, потом `UPDATE ... SET profile_id = 'gh-coordinator-v1'`, потом `SET NOT NULL`. Это последовательный flow трёх SQL — raw SQL точнее выражает intent.

`alembic/env.py` устанавливает `target_metadata = None` намеренно — отключает Alembic autogenerate.

### [5.2] ORM в repositories, raw SQL только когда нужно

Repositories предпочитают `select(...)`, `pg_insert(...)`, `update(...)`, `delete(...)` через SQLAlchemy. **Исключения** — там, где ORM не выражает intent:

- `claim_atomic`: `SELECT FOR UPDATE` через `text(...)` — критически важно для race-safety
- `insert_batch.validation`: `tuple_(channel_id, kind).in_(list_of_pairs)` — ORM хорошо это выражает, но `tuple_` приходится импортировать явно
- `devices.hard_delete`: `DELETE FROM readings/events` через raw SQL потому что они hypertables и не имеют FK на `devices`

### [5.3] Pydantic schemas — strict by default

Все `<Name>In`, `<Name>Patch`, `<Name>Out` имеют `model_config = ConfigDict(extra="forbid")`. Это значит unknown JSON-поля → 422. Полезно когда клиент шлёт устаревшую/опечатанную структуру — лучше упасть рано, чем тихо проигнорировать.

`Out`-модели всегда декорированы `BaseModel.model_validate(orm_obj, from_attributes=True)`. Это позволяет передавать SQLAlchemy ORM объекты прямо в response без ручного маппинга.

### [5.4] `last_used_at` через BackgroundTask

`auth_admin.verify_admin` после успешного lookup планирует `_touch_last_used(token_hash)` через `BackgroundTask`. Это означает:
- HTTP response возвращается СРАЗУ после lookup, без ожидания UPDATE
- UPDATE открывает свой собственный session (через `_db._sessionmaker`), не делит транзакцию с запросом
- Тест `test_last_used_at_updates_in_background` ждёт 200ms перед проверкой

То же самое **не делается** для device bearer (`auth.py`) — `device_credentials` не имеет `last_used_at` (читал ли это нужно — D-3 решит).

### [5.5] testcontainers-postgres: одна сессия = один контейнер

`conftest.py` поднимает `PostgresContainer("timescale/timescaledb:latest-pg16")` с `scope="session"`. Один контейнер = весь тестовый run. Между тестами migrations реапплятся (фикстура `migrated_db` запускает `alembic upgrade head`), но контейнер не пересоздаётся.

**Не использовать `truncate` между тестами** — пишутся тесты в стиле «уникальный device_id / location name» так чтобы не было пересечений.

---

## [6] Cheat-sheet — куда что класть

| Хочешь добавить… | Где |
|---|---|
| Новое поле в existing table | Новая Alembic-миграция + расширить ORM модель + Pydantic-схему |
| Новую таблицу | Новая Alembic-миграция + новый класс в `models.py` |
| Новый тип устройства | Новая миграция с `INSERT INTO device_profiles ...` (см. [§2.2](#22-device-profile-catalog--xiaomi-property)) |
| Новый endpoint | `routes/<name>.py` + регистрация в `routes/__init__.py` + repository если нужен DB-доступ + Pydantic-схемы в `schemas/<name>.py` |
| Новый repository | `repositories/<name>.py` — функции с `AsyncSession` параметром, не классы |
| Изменить existing endpoint | Если меняется wire contract — обновить `<Name>In`/`<Name>Out` + тесты (`tests/test_route_<name>.py`) |
| Бекграунд-задача | Через `fastapi.BackgroundTasks` если нужна после-ответная работа (см. `auth_admin`); если cron-стиль — пока нет инфраструктуры |
| CLI (вне HTTP) | `app/tools/<name>.py` с argparse + asyncio.run |
| Тест чисто-логический | `tests/test_schemas_*.py` или `tests/test_repository_*.py` |
| Тест end-to-end | `tests/test_route_*.py` — httpx ASGI + `wired_app` fixture |
| Глобальная env-переменная | `app/config.py` Settings + соответственно `.env.example` + docker-compose env |

---

## [7] Things to NEVER do

- **Никогда не давать operator'у создавать profiles через HTTP.** Каталог `device_profiles` — единственный «hub knows about devices» механизм; если кто-угодно может создать новый profile, теряется property. Профили добавляются строго через миграции.
- **Никогда не возвращать сырой api_key или admin_token дважды.** Bootstrap / claim возвращают токен **один раз**. После этого сервер хранит только SHA-256 hash. Если operator потерял ключ — единственный путь = revoke + перевыпустить.
- **Никогда не использовать `INSERT ... ON CONFLICT DO NOTHING` для атомного claim.** Race-safety обеспечивается `SELECT ... FOR UPDATE` (см. [§2.4](#24-hue-style-pairing-flow--atomic-claim)). ON CONFLICT не лочит существующие строки на чтении.
- **Никогда не модифицировать existing migration.** Если что-то нужно поменять — пишется новая `000N+1_fix_xxx.py`. Прод/staging уже могут иметь применённые версии.
- **Никогда не использовать sync SQLAlchemy в backend коде.** Всё через `async`. Если попалась библиотека только sync — оборачивать в `asyncio.to_thread(...)` или искать async-версию.
- **Никогда не логировать сырые api_key / admin_token / device_id MAC** через `print` или logger. Принт-statement для debug можно по token_hash[:8] или device_id (он публичный).
- **Никогда не делать commit() в repository функции и потом throw exception.** Транзакция должна быть атомарной — либо все INSERTs прошли + commit, либо ни один. Все наши repos уже следуют этому, но при добавлении новых — следить.
- **Никогда не возвращать `JSONResponse(content=...)` напрямую из endpoint.** Используется `response_model=<Name>Out` для типизированной сериализации. Это даёт автоматический OpenAPI + валидацию + редактируемость для D-3 UI.
- **Никогда не делать `Sensor.last_value` source of truth.** Это денормализованный hot cache. Source of truth — `readings` hypertable; cache можно потерять и пересчитать через `MAX(value) ... WHERE ts = (SELECT MAX(ts) ...)`.

---

## Quick links

- Hub constitution: [`../../.specify/memory/constitution-hub.md`](../../.specify/memory/constitution-hub.md)
- Master constitution: [`../../.specify/memory/constitution.md`](../../.specify/memory/constitution.md)
- Active firmware replan: [`../../docs/superpowers/plans/2026-06-04-multinode-coordinator-replan.md`](../../docs/superpowers/plans/2026-06-04-multinode-coordinator-replan.md)
- Coordinator firmware: [`../../firmware/coordinator/CLAUDE.md`](../../firmware/coordinator/CLAUDE.md)
- Корневые правила: [`../../CLAUDE.md`](../../CLAUDE.md)
