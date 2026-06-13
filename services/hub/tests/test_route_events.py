import time

import pytest
from httpx import ASGITransport, AsyncClient
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.main import app
from app.tools.mint_admin_token import mint_admin_token


@pytest.fixture
def wired_app(monkeypatch, migrated_db):
    from app import db
    monkeypatch.setattr(db, "_engine", migrated_db)
    monkeypatch.setattr(
        db, "_sessionmaker",
        async_sessionmaker(migrated_db, expire_on_commit=False),
    )
    return app


def _bearer(t):
    return {"Authorization": f"Bearer {t}"}


async def _pair_one(ac, admin: str, device_id: str) -> str:
    r = await ac.post(
        "/api/pairing/open", json={"ttl_seconds": 300}, headers=_bearer(admin)
    )
    r2 = await ac.post(
        "/api/pairing/claim",
        json={
            "claim_code": r.json()["code"],
            "device_id":  device_id,
            "mac":        "aa:bb:cc:dd:ee:ff",
            "fw_version": "0.4.0",
            "profile_id": "gh-coordinator-v1",
        },
    )
    return r2.json()["api_key"]


async def _ingest_watered(ac, api_key: str, device_id: str, ts_ms: int) -> None:
    await ac.post(
        "/ingest",
        json={
            "device_id":  device_id,
            "fw_version": "0.4.0",
            "batch_id":   f"evt-{ts_ms}",
            "readings": [
                {"ts": ts_ms, "channel_id": 0, "kind": "air_temp",
                 "value": 21.0, "status": 0},
            ],
            "events": [
                {"ts": ts_ms, "kind": "watered", "payload": {"duration_ms": 3000}},
                {"ts": ts_ms, "kind": "provisioned", "payload": None},
            ],
        },
        headers=_bearer(api_key),
    )


@pytest.mark.asyncio
async def test_events_filters_by_kind(wired_app, migrated_db):
    admin = await mint_admin_token(name="evt-kind")
    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        api_key = await _pair_one(ac, admin, "gh-evtk01")
        await _ingest_watered(ac, api_key, "gh-evtk01", int(time.time() * 1000))

        r = await ac.get(
            "/api/events", params={"kind": "watered"}, headers=_bearer(admin)
        )
        assert r.status_code == 200
        mine = [e for e in r.json() if e["device_id"] == "gh-evtk01"]
        assert mine
        assert all(e["kind"] == "watered" for e in mine)
        assert mine[0]["payload"] == {"duration_ms": 3000}


@pytest.mark.asyncio
async def test_events_requires_admin(wired_app, migrated_db):
    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.get("/api/events")
        assert r.status_code == 401
