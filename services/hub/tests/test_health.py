import pytest
from httpx import ASGITransport, AsyncClient
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.main import app


@pytest.mark.asyncio
async def test_healthz_returns_200(migrated_db, monkeypatch):
    from app import db
    monkeypatch.setattr(db, "_engine", migrated_db)
    monkeypatch.setattr(
        db, "_sessionmaker", async_sessionmaker(migrated_db, expire_on_commit=False)
    )

    async with AsyncClient(transport=ASGITransport(app=app), base_url="http://test") as ac:
        r = await ac.get("/healthz")
    assert r.status_code == 200
    body = r.json()
    assert body["status"] == "ok"
    assert body["db"] == "ok"
