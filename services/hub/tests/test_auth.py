import hashlib
import pytest
from fastapi import Depends, FastAPI
from httpx import ASGITransport, AsyncClient
from sqlalchemy import text
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.auth import AuthenticatedDevice, verify_device


@pytest.fixture
def auth_app(monkeypatch, migrated_db):
    from app import db
    monkeypatch.setattr(db, "_engine", migrated_db)
    monkeypatch.setattr(
        db, "_sessionmaker", async_sessionmaker(migrated_db, expire_on_commit=False)
    )

    app = FastAPI()

    @app.get("/protected")
    async def protected(dev: AuthenticatedDevice = Depends(verify_device)) -> dict:
        return {"device_id": dev.device_id}

    return app


@pytest.mark.asyncio
async def test_missing_token_returns_401(auth_app):
    async with AsyncClient(
        transport=ASGITransport(app=auth_app), base_url="http://test"
    ) as ac:
        r = await ac.get("/protected")
    assert r.status_code == 401


@pytest.mark.asyncio
async def test_invalid_token_returns_401(auth_app):
    async with AsyncClient(
        transport=ASGITransport(app=auth_app), base_url="http://test"
    ) as ac:
        r = await ac.get("/protected", headers={"Authorization": "Bearer wrong"})
    assert r.status_code == 401


@pytest.mark.asyncio
async def test_env_keys_path(auth_app, monkeypatch):
    monkeypatch.setenv("DEVICE_API_KEYS", '{"gh-test": "valid-key"}')
    from app.config import get_settings
    get_settings.cache_clear()

    async with AsyncClient(
        transport=ASGITransport(app=auth_app), base_url="http://test"
    ) as ac:
        r = await ac.get("/protected", headers={"Authorization": "Bearer valid-key"})
    assert r.status_code == 200
    assert r.json() == {"device_id": "gh-test"}


@pytest.mark.asyncio
async def test_db_keys_path(auth_app, migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        await s.execute(text("INSERT INTO devices (device_id) VALUES ('gh-db')"))
        h = hashlib.sha256(b"db-key").hexdigest()
        await s.execute(
            text(
                "INSERT INTO device_credentials (device_id, api_key_hash) VALUES ('gh-db', :h)"
            ),
            {"h": h},
        )
        await s.commit()

    async with AsyncClient(
        transport=ASGITransport(app=auth_app), base_url="http://test"
    ) as ac:
        r = await ac.get("/protected", headers={"Authorization": "Bearer db-key"})
    assert r.status_code == 200
    assert r.json() == {"device_id": "gh-db"}
