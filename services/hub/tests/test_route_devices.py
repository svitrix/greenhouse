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


def _bearer(t): return {"Authorization": f"Bearer {t}"}


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


@pytest.mark.asyncio
async def test_list_includes_paired_device_with_sensors_count(wired_app, migrated_db):
    admin = await mint_admin_token(name="dev-list")

    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        await _pair_one(ac, admin, "gh-listed01")

        r = await ac.get("/api/devices", headers=_bearer(admin))
        assert r.status_code == 200
        rows = r.json()
        match = next((x for x in rows if x["device_id"] == "gh-listed01"), None)
        assert match is not None
        assert match["sensors_count"] == 6
        assert match["profile_id"] == "gh-coordinator-v1"


@pytest.mark.asyncio
async def test_patch_friendly_name_and_location(wired_app, migrated_db):
    admin = await mint_admin_token(name="dev-patch")

    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        await _pair_one(ac, admin, "gh-patched01")
        rl = await ac.post(
            "/api/locations", json={"name": "Дача"}, headers=_bearer(admin)
        )
        loc_id = rl.json()["id"]

        r = await ac.patch(
            "/api/devices/gh-patched01",
            json={"friendly_name": "Главная теплица", "location_id": loc_id},
            headers=_bearer(admin),
        )
    assert r.status_code == 200
    assert r.json()["friendly_name"] == "Главная теплица"
    assert r.json()["location_id"] == loc_id


@pytest.mark.asyncio
async def test_revoke_deletes_credential(wired_app, migrated_db):
    admin = await mint_admin_token(name="dev-revoke")

    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        api_key = await _pair_one(ac, admin, "gh-revoke01")
        # /ingest works now
        import time
        r_before = await ac.post(
            "/ingest",
            json={
                "device_id":  "gh-revoke01",
                "fw_version": "0.4.0",
                "batch_id":   "B1",
                "readings": [
                    {"ts": int(time.time() * 1000), "channel_id": 0,
                     "kind": "air_temp", "value": 22.0, "status": 0},
                ],
                "events": [],
            },
            headers={"Authorization": f"Bearer {api_key}"},
        )
        assert r_before.status_code == 200

        await ac.post(
            "/api/devices/gh-revoke01/revoke", headers=_bearer(admin)
        )

        # /ingest now fails 401 with the same key
        r_after = await ac.post(
            "/ingest",
            json={
                "device_id":  "gh-revoke01",
                "fw_version": "0.4.0",
                "batch_id":   "B2",
                "readings": [
                    {"ts": int(time.time() * 1000), "channel_id": 0,
                     "kind": "air_temp", "value": 22.0, "status": 0},
                ],
                "events": [],
            },
            headers={"Authorization": f"Bearer {api_key}"},
        )
    assert r_after.status_code == 401
