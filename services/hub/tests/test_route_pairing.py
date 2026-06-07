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
async def test_open_requires_admin_token(wired_app):
    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.post("/api/pairing/open", json={"ttl_seconds": 300})
    assert r.status_code == 401


@pytest.mark.asyncio
async def test_full_flow_open_then_claim(wired_app, migrated_db):
    admin = await mint_admin_token(name="route-test")

    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.post(
            "/api/pairing/open",
            json={"ttl_seconds": 300},
            headers={"Authorization": f"Bearer {admin}"},
        )
        assert r.status_code == 200, r.text
        code = r.json()["code"]

        r2 = await ac.post(
            "/api/pairing/claim",
            json={
                "claim_code": code,
                "device_id": "gh-route01",
                "mac": "aa:bb:cc:dd:ee:ff",
                "fw_version": "0.4.0",
                "profile_id": "gh-coordinator-v1",
            },
        )
    assert r2.status_code == 200, r2.text
    body = r2.json()
    assert len(body["api_key"]) == 64
    assert body["device_id"] == "gh-route01"


@pytest.mark.asyncio
async def test_claim_with_wrong_code_returns_410(wired_app):
    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.post(
            "/api/pairing/claim",
            json={
                "claim_code": "000000",
                "device_id": "gh-x",
                "mac": "aa:bb:cc:dd:ee:ff",
                "fw_version": "0.4.0",
                "profile_id": "gh-coordinator-v1",
            },
        )
    assert r.status_code == 410


@pytest.mark.asyncio
async def test_list_windows_requires_admin(wired_app):
    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.get("/api/pairing/windows")
    assert r.status_code == 401


@pytest.mark.asyncio
async def test_claim_with_unknown_profile_returns_422(wired_app, migrated_db):
    admin = await mint_admin_token(name="prof-test")

    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.post(
            "/api/pairing/open",
            json={"ttl_seconds": 300},
            headers={"Authorization": f"Bearer {admin}"},
        )
        code = r.json()["code"]
        r2 = await ac.post(
            "/api/pairing/claim",
            json={
                "claim_code": code,
                "device_id":  "gh-future01",
                "mac":        "aa:bb:cc:dd:ee:ff",
                "fw_version": "9.9.9",
                "profile_id": "gh-fictional-v9",
            },
        )
    assert r2.status_code == 422
    assert "not registered" in r2.json()["detail"]


@pytest.mark.asyncio
async def test_claim_seeds_sensors_from_profile(wired_app, migrated_db):
    admin = await mint_admin_token(name="seed-test")

    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.post(
            "/api/pairing/open",
            json={"ttl_seconds": 300},
            headers={"Authorization": f"Bearer {admin}"},
        )
        code = r.json()["code"]
        r2 = await ac.post(
            "/api/pairing/claim",
            json={
                "claim_code": code,
                "device_id":  "gh-seeded01",
                "mac":        "aa:bb:cc:dd:ee:ff",
                "fw_version": "0.4.0",
                "profile_id": "gh-coordinator-v1",
            },
        )
    assert r2.status_code == 200

    from sqlalchemy import text
    from sqlalchemy.ext.asyncio import async_sessionmaker
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        count = (
            await s.execute(
                text("SELECT count(*) FROM sensors WHERE device_id='gh-seeded01'")
            )
        ).scalar_one()
    assert count == 6
