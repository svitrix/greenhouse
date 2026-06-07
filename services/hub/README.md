# Greenhouse Telemetry Backend

FastAPI + PostgreSQL/TimescaleDB. Receives batched sensor readings from coordinator firmware over HTTPS, persists them for analytics and (future) ML.

Spec: [`../../docs/superpowers/specs/2026-06-01-telemetry-backend-design.md`](../../docs/superpowers/specs/2026-06-01-telemetry-backend-design.md).

## Local dev

```sh
make up        # docker compose up -d --build
make migrate   # alembic upgrade head
make test      # pytest
make psql      # interactive psql shell
make down      # stop containers (keeps volume)
```

Wire format and auth model are documented in spec §4.4–§4.6.
