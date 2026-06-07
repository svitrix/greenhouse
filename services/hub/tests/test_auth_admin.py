import asyncio
import hashlib

import pytest
from fastapi import Depends, FastAPI
from httpx import ASGITransport, AsyncClient
from sqlalchemy import select
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.auth_admin import AuthenticatedAdmin, verify_admin
from app.models import AdminToken
from app.tools.mint_admin_token import mint_admin_token


@pytest.fixture
def admin_app(monkeypatch, migrated_db):
    from app import db
    monkeypatch.setattr(db, "_engine", migrated_db)
    monkeypatch.setattr(
        db, "_sessionmaker",
        async_sessionmaker(migrated_db, expire_on_commit=False),
    )
    app = FastAPI()

    @app.get("/admin-only")
    async def admin_only(admin: AuthenticatedAdmin = Depends(verify_admin)) -> dict:
        return {"name": admin.name or "anonymous"}

    return app


@pytest.mark.asyncio
async def test_missing_token_is_401(admin_app):
    async with AsyncClient(
        transport=ASGITransport(app=admin_app), base_url="http://t"
    ) as ac:
        r = await ac.get("/admin-only")
    assert r.status_code == 401


@pytest.mark.asyncio
async def test_unknown_token_is_401(admin_app):
    async with AsyncClient(
        transport=ASGITransport(app=admin_app), base_url="http://t"
    ) as ac:
        r = await ac.get("/admin-only", headers={"Authorization": "Bearer wrong"})
    assert r.status_code == 401


@pytest.mark.asyncio
async def test_valid_token_returns_200(admin_app, migrated_db):
    token = await mint_admin_token(name="browser-A")
    async with AsyncClient(
        transport=ASGITransport(app=admin_app), base_url="http://t"
    ) as ac:
        r = await ac.get(
            "/admin-only", headers={"Authorization": f"Bearer {token}"}
        )
    assert r.status_code == 200
    assert r.json() == {"name": "browser-A"}


@pytest.mark.asyncio
async def test_last_used_at_updates_in_background(admin_app, migrated_db):
    token = await mint_admin_token(name="touch-test")
    digest = hashlib.sha256(token.encode()).hexdigest()

    async with AsyncClient(
        transport=ASGITransport(app=admin_app), base_url="http://t"
    ) as ac:
        await ac.get("/admin-only", headers={"Authorization": f"Bearer {token}"})

    # BackgroundTask runs after response; give it a moment
    await asyncio.sleep(0.2)

    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        row = (
            await s.execute(
                select(AdminToken.last_used_at).where(
                    AdminToken.token_hash == digest
                )
            )
        ).scalar_one()
    assert row is not None
