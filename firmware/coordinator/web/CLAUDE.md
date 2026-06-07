# CLAUDE.md — `firmware/coordinator/web/`

> Preact + TS + Vite SPA served from the coordinator's LittleFS partition over HTTP.
> Reads `/api/*` from the same origin in production; Vite dev server proxies in dev.
> Routing, asset cache, and Wi-Fi/MQTT setup contracts live here — global firmware
> rules in [`../CLAUDE.md`](../CLAUDE.md) and root [`../../../CLAUDE.md`](../../../CLAUDE.md).

## Routing — hash mode is a load-bearing contract

The SPA uses **hash-based routing** (`#/setup`, `#/`). This is not an accident — it
is the only mode that survives browser refresh on a deep link without server-side
support. With hash routing, every refresh hits `GET /` regardless of which screen
the user was on, and the SPA decides what to render from `location.hash` after boot.

**Do NOT migrate to History API routing without a coordinated server change.**
History mode would mean a refresh on `/settings` issues `GET /settings`, which
requires the server to serve `index.html` as a fallback. The fallback exists in
`RestApi::start()` (`server_.onNotFound(...)`), but switching modes is a contract
change that needs review — silent migration would only fail for users who refresh
on a non-root URL, making the bug hard to spot in dev.

If a router library is ever added, use hash mode explicitly:
- preact-router: import `createHashHistory` from `history` and pass to `<Router>`
- wouter-preact: use the `useHashLocation` hook
- preact-iso: verify hash-mode support before adopting; may force History API

## Auth — HTTP Basic, in-app login

The firmware enforces HTTP Basic Auth on every `/api/*` route. The SPA does
**not** rely on the browser's native "Sign in" popup — modern browsers do not
surface that dialog for `fetch()`/XHR, so relying on it left the app unable to
authenticate (every `/api/*` → 401 → blank/no data).

Instead the SPA owns auth explicitly:
- `src/api/auth.ts` stores credentials in `sessionStorage` (in-memory fallback
  when storage is unavailable), exposes `basicHeader()`, and the `needsLogin` /
  `loginError` signals.
- `src/api/client.ts` attaches `Authorization: Basic …` to every request and, on
  a `401`, clears the creds and flips `needsLogin` so the app routes to `<Login>`
  (instead of throwing into a blank screen).
- `src/App/App.tsx` probes `GET /api/status` to pick the mode: `401` ⇒
  operational + show `<Login>`; `200 mode:provisioning` ⇒ Provisioning (no
  login); `200 mode:operational` ⇒ authenticated dashboard.
- `src/routes/Login/Login.tsx` is the login form; Dashboard has a **Sign out**
  button (`clearCreds()` + `needsLogin = true`).

Static handlers (`/`, `/assets/*`, `/icons.svg`) remain unauthenticated so the
SPA shell loads before login. Provisioning-mode endpoints (`/api/status`,
`/scan`, `/save`) are open and must stay reachable without credentials.
