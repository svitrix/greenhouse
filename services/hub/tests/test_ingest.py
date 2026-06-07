import time

import pytest
from httpx import ASGITransport, AsyncClient
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.main import app


def _bearer(key: str) -> dict[str, str]:
    return {"Authorization": f"Bearer {key}"}


def _payload(device_id: str = "gh-a1b2c3d4", ts: int | None = None) -> dict:
    return {
        "device_id": device_id,
        "fw_version": "1.0.0",
        "batch_id": "01HF",
        "readings": [
            {
                "ts": ts or int(time.time() * 1000),
                "channel_id": 0,
                "kind": "air_temp",
                "value": 22.5,
                "status": 0,
            }
        ],
        "events": [],
    }


@pytest.fixture
def app_with_db(monkeypatch, migrated_db):
    from app import db
    monkeypatch.setattr(db, "_engine", migrated_db)
    monkeypatch.setattr(
        db, "_sessionmaker", async_sessionmaker(migrated_db, expire_on_commit=False)
    )
    monkeypatch.setenv("DEVICE_API_KEYS", '{"gh-a1b2c3d4": "dev-secret-key"}')
    from app.config import get_settings
    get_settings.cache_clear()
    return app


@pytest.mark.asyncio
async def test_ingest_happy_path(app_with_db):
    async with AsyncClient(
        transport=ASGITransport(app=app_with_db), base_url="http://t"
    ) as ac:
        r = await ac.post("/ingest", json=_payload(), headers=_bearer("dev-secret-key"))
    assert r.status_code == 200, r.text
    body = r.json()
    assert body["accepted_readings"] == 1
    assert body["duplicates_skipped"] == 0


@pytest.mark.asyncio
async def test_ingest_rejects_missing_token(app_with_db):
    async with AsyncClient(
        transport=ASGITransport(app=app_with_db), base_url="http://t"
    ) as ac:
        r = await ac.post("/ingest", json=_payload())
    assert r.status_code == 401


@pytest.mark.asyncio
async def test_ingest_rejects_device_id_mismatch(app_with_db):
    async with AsyncClient(
        transport=ASGITransport(app=app_with_db), base_url="http://t"
    ) as ac:
        r = await ac.post(
            "/ingest",
            json=_payload(device_id="gh-someone-else"),
            headers=_bearer("dev-secret-key"),
        )
    assert r.status_code == 403


@pytest.mark.asyncio
async def test_ingest_rejects_far_future_ts(app_with_db):
    far_future = int(time.time() * 1000) + 30 * 24 * 3600 * 1000  # +30 days
    async with AsyncClient(
        transport=ASGITransport(app=app_with_db), base_url="http://t"
    ) as ac:
        r = await ac.post(
            "/ingest", json=_payload(ts=far_future), headers=_bearer("dev-secret-key")
        )
    assert r.status_code == 400


@pytest.mark.asyncio
async def test_ingest_dedups_replay(app_with_db):
    p = _payload()
    async with AsyncClient(
        transport=ASGITransport(app=app_with_db), base_url="http://t"
    ) as ac:
        r1 = await ac.post("/ingest", json=p, headers=_bearer("dev-secret-key"))
        r2 = await ac.post("/ingest", json=p, headers=_bearer("dev-secret-key"))
    assert r1.status_code == r2.status_code == 200
    assert r1.json()["accepted_readings"] == 1
    assert r2.json()["accepted_readings"] == 0
    assert r2.json()["duplicates_skipped"] == 1
