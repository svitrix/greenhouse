import time

import pytest
from httpx import ASGITransport, AsyncClient
from sqlalchemy import text
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
async def test_last_value_reflects_max_ts_per_triplet(wired_app, migrated_db):
    admin = await mint_admin_token(name="denorm-test")

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
                "device_id":  "gh-denorm01",
                "mac":        "aa:bb:cc:dd:ee:ff",
                "fw_version": "0.4.0",
                "profile_id": "gh-coordinator-v1",
            },
        )
        api_key = r2.json()["api_key"]

        # Two readings, the second is newer.
        ts1 = int(time.time() * 1000) - 5000
        ts2 = int(time.time() * 1000)
        await ac.post(
            "/ingest",
            json={
                "device_id":  "gh-denorm01",
                "fw_version": "0.4.0",
                "batch_id":   "D1",
                "readings": [
                    {"ts": ts1, "channel_id": 0, "kind": "air_temp",
                     "value": 20.0, "status": 0},
                    {"ts": ts2, "channel_id": 0, "kind": "air_temp",
                     "value": 22.5, "status": 0},
                ],
                "events": [],
            },
            headers={"Authorization": f"Bearer {api_key}"},
        )

    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        last = (
            await s.execute(
                text(
                    "SELECT last_value FROM sensors "
                    "WHERE device_id='gh-denorm01' "
                    "  AND channel_id=0 AND kind='air_temp'"
                )
            )
        ).scalar_one()
    assert last == 22.5
