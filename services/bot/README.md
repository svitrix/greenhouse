# `services/bot/` — Greenhouse Telegram bot

A standalone Telegram bot that talks to the hub **only over its REST API**
(no database access). It:

- 🔔 notifies subscribers when watering events arrive at the hub
  (`watered`, `dry_run_aborted`);
- 📊 shows the latest sensor readings per device;
- 💧 turns the pump on/off via inline buttons, which enqueue a command on
  the hub (`POST /api/devices/{id}/commands`);
- 🌐 speaks **English and Ukrainian** — see [Localisation](#localisation).

> **Pump control depends on the reverse channel.** The bot only *enqueues*
> commands. The coordinator firmware must poll
> `GET /api/devices/{id}/commands/pending` and `POST /api/commands/{id}/ack`
> to actually act on them — that firmware work is tracked separately (D-5).
> Until then, buttons queue commands that sit in `pending`.
>
> **Notifications are batch-delayed.** Watering events reach the hub only on
> the coordinator's `/ingest` flush (~15 min), so a ping can lag by that
> much. A real-time push would need a firmware change.

## Stack & requirements

- **Python 3.14** (same version as `services/hub/`; `requires-python = ">=3.14"`)
- [python-telegram-bot](https://docs.python-telegram-bot.org) 22.x (`[job-queue]` extra) — long-polling + the notification job
- [httpx](https://www.python-httpx.org) — async hub API client
- [pydantic](https://docs.pydantic.dev) / pydantic-settings — env-driven config

No inbound port: the bot dials out to Telegram (long-polling) and to the hub.

## Architecture

```text
Telegram  ──long-polling──►  bot (this service)  ──REST (admin token)──►  hub
                                  │
                          PicklePersistence (/data)
                          subscribers · language · event cursor
```

Subscribers, each chat's language, and the notification cursor are the bot's
own state, persisted via `python-telegram-bot`'s `PicklePersistence` into the
`/data` volume. The hub stays the source of truth for devices, sensors, and
the command queue.

## Project layout

```text
bot/
├── app.py              # composition root: builds the Application, wires handlers + the poll job
├── __main__.py         # `python -m bot` entrypoint
├── config.py           # pydantic-settings (env)
├── clients/
│   └── hub.py          # async REST client over the hub admin API
├── core/
│   ├── auth.py         # Telegram user-id allowlist (fail-closed)
│   └── store.py        # bot_data accessors: subscribers / language / cursor
├── handlers/
│   ├── __init__.py     # register(application) — wires handlers onto the app
│   ├── common.py       # shared helpers: access control, language, hub accessor
│   ├── commands.py     # /start /menu /sensors /language
│   └── callbacks.py    # inline-button router + per-screen handlers
├── ui/
│   ├── keyboards.py    # inline-keyboard builders (localised)
│   └── formatting.py   # pure HTML message rendering (localised, html-escaped)
├── jobs/
│   └── notifier.py     # JobQueue tick: poll hub events → push to subscribers
└── i18n/
    ├── __init__.py     # JSON catalog loader + t()
    └── locales/        # en.json, uk.json
```

Layers depend inward: `handlers` → `ui` / `clients` / `core` / `i18n`;
`ui` → `i18n`; `core` and `clients` depend on nothing bot-specific. The
composition root (`app.py`) is the only place that knows about all of them.

## Configuration

Copy `.env.example` → `.env`. Key vars:

| Var | Meaning |
|---|---|
| `TELEGRAM_BOT_TOKEN` | from @BotFather |
| `TELEGRAM_ALLOWED_USER_IDS` | comma-separated allowlist of Telegram user ids; **empty = deny everyone** |
| `HUB_API_URL` | hub base URL (compose: `http://hub:8000`) |
| `HUB_ADMIN_TOKEN` | hub admin bearer token (see below) |
| `BOT_POLL_SECONDS` | event poll interval (default 30) |
| `PERSISTENCE_PATH` | pickle file path (default `/data/bot_persistence.pkl`) |

### Security note

The bot holds a **hub admin token**, so it can do anything an admin can. Keep
the token out of logs and source control, run the bot on the trusted network,
and restrict `TELEGRAM_ALLOWED_USER_IDS` to your own account(s). This matches
the project's home-DIY threat model (see root `CLAUDE.md §9.5`).

Mint a token (from the hub):

```bash
docker compose exec hub python -m app.tools.mint_admin_token --name telegram-bot
```

## Run

Needs **Python 3.14**.

```bash
# Local (needs a reachable hub):
python3.14 -m venv .venv && .venv/bin/pip install -e ".[dev]"
HUB_API_URL=http://localhost:8000 HUB_ADMIN_TOKEN=... \
  TELEGRAM_BOT_TOKEN=... TELEGRAM_ALLOWED_USER_IDS=123456 \
  .venv/bin/python -m bot

# Docker Compose (from services/hub/):
TELEGRAM_BOT_TOKEN=... TELEGRAM_ALLOWED_USER_IDS=123456 HUB_ADMIN_TOKEN=... \
  docker compose up -d --build bot
docker compose logs -f bot     # or: make bot-logs
```

## Test

```bash
.venv/bin/pytest -q          # pure unit tests, no network/Docker
.venv/bin/ruff check .
```

The suite covers the hub client (against `httpx.MockTransport`), message
formatting, the allowlist, the subscriber/language store, and i18n catalog
parity — no live Telegram or hub needed.

## Bot commands & menu

- `/start` — subscribe to notifications + open the menu
- `/menu` — main menu (My beds · Notifications · Language)
- `/sensors` — pick a device, open its card
- `/language` — switch language (English / Українська)

The menu is inline-button driven. **🪴 My beds** → pick a device → a single
**device card**: online status + last-seen, every sensor reading tagged with a
qualitative band (`✅ optimal`, `⚠️ dry · water me`, …), and ▶️/⏹ pump controls
plus a 🔄 refresh — all on one screen. **🔔 Notifications** → toggle watering
pings on/off; **🌐 Language** → switch.

The band labels and thresholds are ported verbatim from the web dashboard
([`services/dashboard/src/lib/sensor-state.ts`](../dashboard/src/lib/sensor-state.ts))
into [bot/ui/sensor_state.py](bot/ui/sensor_state.py), so the bot and the UI
agree on what "dry" or "optimal" means.

## Localisation

Two languages: **English** and **Ukrainian**. On first `/start` the bot picks
the language from the user's Telegram client (`language_code`), defaulting to
English; the user can change it any time via `/language` or the 🌐 menu button.
The choice is stored per chat in the bot's persistence.

Translations live in one JSON file per language under
[bot/i18n/locales/](bot/i18n/locales/) (`en.json`, `uk.json`) — no gettext /
`.mo` build step; [bot/i18n/](bot/i18n/) loads them at startup (and they ship
in the wheel / image). To add a language: drop `i18n/locales/<code>.json` with
the full key set (including a `language_name` key with its flag) and add
`<code>` to `SUPPORTED` in `i18n/__init__.py`. The
`test_every_key_exists_in_all_languages` test enforces that every locale has
the same keys as English.
