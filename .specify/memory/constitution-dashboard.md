# Greenhouse ESP32 — Dashboard Constitution

> Governs: `services/dashboard/`
> React 18 · TypeScript · Vite · shadcn/ui · Tailwind CSS 3
> TanStack Query v5 · Zustand · React Hook Form · Zod · React Router v6
>
> Parent: [`.specify/memory/constitution.md`](constitution.md)

## Core Principles

### I. Design Language — Flat Shadcn, Greenhouse Palette

The visual language is flat, functional, and plant-adjacent. Reference: the existing
`services/dashboard` codebase is the canonical design source — not any external
prototype or Figma file.

- **Color palette**: teal (primary), lime/mint (accent), warm-grey (background),
  amber/rose (warnings/errors)
- **Components**: shadcn/ui primitives only — no third-party component libraries
- **Typography**: system fonts only (`font-sans` Tailwind default) — no CDN web fonts
- **Spacing**: Tailwind spacing scale strictly — no arbitrary `px-[13px]` values
- **Icons**: `lucide-react` only — no icon CDNs, no inline SVG for UI icons

**MUST NOT**: add a new component library (MUI, Ant, Mantine, etc.) alongside shadcn.
**MUST NOT**: use `<style>` blocks or CSS modules where a Tailwind utility suffices.

### II. Component Architecture — Co-located, Flat Hierarchy

One component = one file. No barrel `index.ts` files that re-export a folder's entire
contents. Shared primitives live in `src/components/ui/` (shadcn-generated). Feature
components live in `src/components/` or `src/pages/`. No component deeper than two
nesting levels (`pages/` → `components/` → `ui/`).

Props interfaces are defined in the same file as the component — not in a separate
`types.ts` unless shared across more than two components.

**MUST NOT**: put API calls, `useQuery`, or `useMutation` hooks directly in
leaf/presentational components. Data-fetching belongs in page-level or hook files.

### III. Server State — TanStack Query, No Custom Fetch Wrappers

All server state (hub API responses) uses TanStack Query v5:
`useQuery` for reads, `useMutation` for writes. Query keys follow the
`[resource, id?, params?]` tuple convention.

Client-only UI state (modals, selections, form drafts) uses Zustand stores defined
in `src/lib/store/`. No `useState` for state that needs to survive component unmount
or be shared across two unrelated components.

**MUST NOT**: write a custom `useFetch` abstraction that bypasses TanStack Query.
**MUST NOT**: put server responses in a Zustand store — that creates a stale
second cache alongside TanStack Query's cache.

### IV. Schema-Driven Validation — Zod at Every API Boundary

All API response shapes are validated with Zod schemas defined in `src/schemas/`.
`useQuery`'s `select` or a thin wrapper validates the raw response before it reaches
components. If the API changes shape unexpectedly, the schema throws — not a
downstream render crash.

Form input validation uses `react-hook-form` + `@hookform/resolvers/zod`. No
imperative `if (!value) setError(...)` checks inside `onSubmit`.

TypeScript strict mode (`"strict": true`) is non-negotiable. No `as any`, no `!`
non-null assertions without an accompanying comment explaining why it is safe.

**MUST NOT**: use `response.json()` without schema validation in any query function.
**MUST NOT**: disable TypeScript strict checks via `@ts-ignore` without a comment.

### V. Accessibility — WCAG AA, Mobile-First

- Minimum contrast ratio: 4.5:1 for body text, 3:1 for large text (WCAG AA)
- Status indicators MUST use BOTH color AND icon/text label — never color alone
  (colorblind users)
- All interactive elements MUST have accessible labels (`aria-label` or visible text)
- Mobile-first: baseline viewport is 375 px (iPhone SE). All layouts MUST work at
  375 px before being tested at wider breakpoints
- Touch targets: minimum 44 × 44 px

**MUST NOT**: convey state (error, warning, online/offline) through color alone.

---

## Testing

- Component tests: `@testing-library/react` + `vitest` + `jsdom`
- Query/mutation tests: mock via `msw` or TanStack Query's `QueryClient` with
  mock data — no network calls in unit tests
- No snapshot tests — they break on every visual tweak and add no behavioral coverage

Every new page-level component ships with at least a "renders without crashing" test.
New form flows ship with a submit-path test.

---

## Build & Dev

```bash
npm run dev        # Vite dev server
npm run build      # Production build → dist/
npm run test       # vitest run
npm run typecheck  # tsc --noEmit
```

`npm run typecheck` MUST be green before any merge. `eslint` errors are also blocking.

---

## Governance

Defers to the Master Constitution for amendment procedure and version policy.
Sub-constitution version is independent; bump it when dashboard-specific rules change.
Reference: [`.specify/memory/constitution.md`](constitution.md)

**Version**: 1.0.0 | **Ratified**: 2026-06-05 | **Last Amended**: 2026-06-07
