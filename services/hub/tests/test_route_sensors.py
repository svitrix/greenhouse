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


async def _pair(ac, admin, device_id):
    r = await ac.post(
        "/api/pairing/open", json={"ttl_seconds": 300}, headers=_bearer(admin)
    )
    await ac.post(
        "/api/pairing/claim",
        json={
            "claim_code": r.json()["code"],
            "device_id":  device_id,
            "mac":        "aa:bb:cc:dd:ee:ff",
            "fw_version": "0.4.0",
            "profile_id": "gh-coordinator-v1",
        },
    )


@pytest.mark.asyncio
async def test_list_returns_six_for_paired_device(wired_app, migrated_db):
    admin = await mint_admin_token(name="sens-list")

    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        await _pair(ac, admin, "gh-sense01")

        r = await ac.get(
            "/api/sensors?device_id=gh-sense01", headers=_bearer(admin)
        )
    assert r.status_code == 200
    kinds = sorted({x["kind"] for x in r.json()})
    assert kinds == [
        "air_humidity", "air_temp", "battery_pct", "battery_v",
        "soil_moist", "soil_temp",
    ]


@pytest.mark.asyncio
async def test_patch_friendly_name_and_calibration(wired_app, migrated_db):
    admin = await mint_admin_token(name="sens-patch")

    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        await _pair(ac, admin, "gh-spatch01")
        r = await ac.patch(
            "/api/sensors/gh-spatch01/1/soil_moist",
            json={
                "friendly_name": "Помидор #3 front",
                "calibration_json": {"raw_dry": 249, "raw_wet": 489},
            },
            headers=_bearer(admin),
        )
    assert r.status_code == 200
    assert r.json()["friendly_name"] == "Помидор #3 front"
    assert r.json()["calibration_json"] == {"raw_dry": 249, "raw_wet": 489}
