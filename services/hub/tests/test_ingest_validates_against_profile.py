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


@pytest.mark.asyncio
async def test_unknown_triplet_is_rejected_400(wired_app, migrated_db):
    admin = await mint_admin_token(name="ingest-prof-test")

    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.post(
            "/api/pairing/open", json={"ttl_seconds": 300},
            headers={"Authorization": f"Bearer {admin}"},
        )
        r2 = await ac.post(
            "/api/pairing/claim",
            json={
                "claim_code": r.json()["code"],
                "device_id":  "gh-validate01",
                "mac":        "aa:bb:cc:dd:ee:ff",
                "fw_version": "0.4.0",
                "profile_id": "gh-coordinator-v1",
            },
        )
        api_key = r2.json()["api_key"]

        # channel_id=99 with kind=air_temp is unknown for this device.
        ts = int(time.time() * 1000)
        r3 = await ac.post(
            "/ingest",
            json={
                "device_id":  "gh-validate01",
                "fw_version": "0.4.0",
                "batch_id":   "X1",
                "readings": [
                    {"ts": ts, "channel_id": 99, "kind": "air_temp",
                     "value": 23.0, "status": 0},
                ],
                "events": [],
            },
            headers={"Authorization": f"Bearer {api_key}"},
        )
    assert r3.status_code == 400, r3.text
    assert "unknown_sensor_for_device" in r3.json()["detail"]


@pytest.mark.asyncio
async def test_known_triplet_still_accepted(wired_app, migrated_db):
    admin = await mint_admin_token(name="ingest-known-test")

    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.post(
            "/api/pairing/open", json={"ttl_seconds": 300},
            headers={"Authorization": f"Bearer {admin}"},
        )
        r2 = await ac.post(
            "/api/pairing/claim",
            json={
                "claim_code": r.json()["code"],
                "device_id":  "gh-known01",
                "mac":        "aa:bb:cc:dd:ee:ff",
                "fw_version": "0.4.0",
                "profile_id": "gh-coordinator-v1",
            },
        )
        api_key = r2.json()["api_key"]

        ts = int(time.time() * 1000)
        r3 = await ac.post(
            "/ingest",
            json={
                "device_id":  "gh-known01",
                "fw_version": "0.4.0",
                "batch_id":   "X2",
                "readings": [
                    {"ts": ts, "channel_id": 0, "kind": "air_temp",
                     "value": 23.0, "status": 0},
                ],
                "events": [],
            },
            headers={"Authorization": f"Bearer {api_key}"},
        )
    assert r3.status_code == 200, r3.text
    assert r3.json()["accepted_readings"] == 1
