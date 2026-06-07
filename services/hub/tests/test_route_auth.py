import time

import pytest
from httpx import ASGITransport, AsyncClient
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.main import app
from app.tools.mint_admin_user import mint_admin_user


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
async def test_login_happy_path(wired_app, migrated_db):
    await mint_admin_user("alice", "alicepass1234")
    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.post(
            "/api/auth/login",
            json={"username": "alice", "password": "alicepass1234"},
        )
    assert r.status_code == 200
    body = r.json()
    assert len(body["admin_token"]) == 64
    assert body["name"].startswith("login-alice-")


@pytest.mark.asyncio
async def test_login_wrong_password_returns_401(wired_app, migrated_db):
    await mint_admin_user("bob", "rightpass1234")
    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.post(
            "/api/auth/login",
            json={"username": "bob", "password": "wrongpass1234"},
        )
    assert r.status_code == 401


@pytest.mark.asyncio
async def test_login_unknown_user_returns_401(wired_app):
    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.post(
            "/api/auth/login",
            json={"username": "ghost", "password": "ghostpass1234"},
        )
    assert r.status_code == 401


@pytest.mark.asyncio
async def test_login_unknown_user_is_not_instant(wired_app):
    # Constant-time check: unknown user must still run argon2 verify, so
    # it should take a comparable amount of wall-clock time to the happy
    # path. 20 ms is a safe lower bound for argon2 default params.
    start = time.perf_counter()
    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        await ac.post(
            "/api/auth/login",
            json={"username": "ghost", "password": "ghostpass1234"},
        )
    elapsed = time.perf_counter() - start
    assert elapsed > 0.020, f"unknown-user login was suspiciously fast: {elapsed}s"


@pytest.mark.asyncio
async def test_login_minted_token_works_against_existing_admin_endpoints(
    wired_app, migrated_db,
):
    await mint_admin_user("carol", "carolpass1234")
    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.post(
            "/api/auth/login",
            json={"username": "carol", "password": "carolpass1234"},
        )
        token = r.json()["admin_token"]
        r2 = await ac.get(
            "/api/devices", headers={"Authorization": f"Bearer {token}"}
        )
    assert r2.status_code == 200
