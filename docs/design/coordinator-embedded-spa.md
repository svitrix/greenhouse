# Coordinator Embedded SPA — Design Reference

> **Status:** Implemented (v2.0.0-multinode)
> **Location:** `firmware/coordinator/web/`
> **Stack:** Preact + TypeScript + Vite → LittleFS
> **Served by:** ESPAsyncWebServer from LittleFS partition

---

## What it is

A lightweight single-page application embedded in the coordinator's LittleFS flash
partition. It replaces the original vanilla-JS v1 dashboard and covers both the
provisioning wizard and the operational multi-node dashboard.

The SPA is built on a dev machine (`npm run build`) and uploaded to LittleFS
(`pio run -t uploadfs`). The coordinator serves `index.html` + assets from `/`.

---

## Technology choices

### Preact over vanilla JS

The original v1 brief called for vanilla JS + Preact with no build step. The v2 rewrite
uses Preact + TypeScript + Vite for three reasons:

1. **Multi-route SPA complexity.** 6 routes, shared state (auth, registry snapshot,
   SSE stream) — managing this without a component model produces spaghetti.
2. **Type safety.** TypeScript catches REST contract mismatches at compile time. The
   API surface grew from 6 endpoints (v1) to 13 (v2) — types pay for themselves.
3. **Signals for state.** Preact Signals gives fine-grained reactivity with minimal
   re-renders — important for live SSE updates on low-power-client targets.

Build output is ≤ 200 KB gzipped (enforced in Vite config). System fonts only; no CDN.

---

## Hash-based routing (load-bearing constraint)

All routes use `location.hash`:

| Hash | Component | Purpose |
|---|---|---|
| `#/` | Dashboard | All nodes, pump status, auto-water decision |
| `#/nodes/{ieee}` | NodeDetail | Per-node 24-h history, alias editor, delete |
| `#/settings` | Settings | Config snapshot + update (WiFi, MQTT, soil cal, admin pw) |
| `#/setup` | Provisioning | First-boot wizard + analytics URL |
| `#/pair` | Pair | Hub pairing (6-digit claim code flow) |
| `#/login` | Login | HTTP Basic credentials form |

**Why hash-based, not History API:**
The coordinator serves every URL as `GET /` → `index.html` from LittleFS. The
ESPAsyncWebServer catch-all route (`server.onNotFound`) returns `index.html` for any
unknown path. With History API routes (`/nodes/00:11:...`), a hard refresh sends
`GET /nodes/00:11:...` to the firmware — this requires server-side redirect logic.
Hash routes never hit the server: `GET /` always loads the SPA, then JS reads
`location.hash`. This is a **load-bearing architectural constraint** — do not switch to
History API without updating the `onNotFound` handler.

---

## Authentication

HTTP Basic auth. Credentials are stored in `sessionStorage` and attached to every
`/api/*` request via the `Authorization: Basic <b64>` header.

On `401` from any `/api/*` call: clear `sessionStorage`, route to `#/login`.

**Why Basic over Bearer token:**
- No token storage or refresh logic.
- Admin credentials are already provisioned into NVS during the setup wizard.
- LAN-only deployment; TLS termination is optional (insecure mode allowed via NVS flag
  per §9.5 of the root constitution — home DIY threat model).

---

## Real-time updates (SSE)

On operational mode entry, the SPA opens an `EventSource` to `/api/events`. The
coordinator pushes a `dashboard` event every 2 seconds containing the full node
registry snapshot + pump state + auto-water decision.

**Fallback:** on `EventSource` error or unsupported browser, the SPA falls back to
polling `GET /api/dashboard` every 5 seconds.

SSE `id` is monotonically incremented. The SPA uses `Last-Event-ID` header on
reconnect to allow the firmware to detect missed events (currently a no-op — firmware
always sends full snapshot, not deltas).

---

## Design language

Inherited from `services/dashboard` (the React admin hub UI):

- **Primary**: teal (`#0d9488` / `teal-600`)
- **Accent**: lime/mint (`#65a30d` / `lime-600`)
- **Background**: warm grey (`#fafaf9` / `stone-50`)
- **Danger**: rose (`#e11d48` / `rose-600`)
- **Status chips**: color + icon label (never color alone — WCAG AA, colorblind safe)
- **Fonts**: system stack (`font-family: system-ui, -apple-system, sans-serif`)
- **No CDN, no external fonts**

Mobile-first. Baseline tested at 375 px (iPhone SE). Cards stack vertically below
`sm:` breakpoint.

---

## REST API surface used by the SPA

| Endpoint | Method | Used by |
|---|---|---|
| `/api/status` | GET | Mode detection, auth check |
| `/api/nodes` | GET | Dashboard node list |
| `/api/nodes/{ieee}` | GET | NodeDetail |
| `/api/nodes/{ieee}/alias` | PUT | NodeDetail alias editor |
| `/api/nodes/{ieee}` | DELETE | NodeDetail two-step delete |
| `/api/history` | GET | NodeDetail history chart |
| `/api/pump` | GET / POST | Dashboard pump card |
| `/api/config` | GET / POST | Settings page |
| `/api/auto-water/state` | GET | Dashboard auto-water card |
| `/api/zigbee/permit-join` | POST | Settings page |
| `/api/dashboard` | GET | SSE fallback, initial load |
| `/api/events` | SSE | Live dashboard |
| `/save` | POST | Provisioning form submit |
| `/api/pairing/claim` | POST | `#/pair` hub claim |

---

## Build & upload

```bash
# Build SPA (run from firmware/coordinator/web/)
npm run build          # → dist/

# Upload to coordinator
pio run -t uploadfs -e coordinator -d firmware/coordinator

# Dev mode (mocks API via Vite proxy to a running coordinator on LAN)
VITE_API_HOST=http://192.168.x.x npm run dev
```
