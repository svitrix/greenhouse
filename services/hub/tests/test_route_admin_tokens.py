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


@pytest.mark.asyncio
async def test_list_and_revoke(wired_app, migrated_db):
    import hashlib
    admin1 = await mint_admin_token(name="primary")
    admin2 = await mint_admin_token(name="secondary")

    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.get("/api/admin/tokens", headers=_bearer(admin1))
        assert r.status_code == 200
        names = {x["name"] for x in r.json()}
        assert {"primary", "secondary"}.issubset(names)

        # revoke secondary by hash prefix
        prefix2 = hashlib.sha256(admin2.encode()).hexdigest()[:8]
        r2 = await ac.delete(
            f"/api/admin/tokens/{prefix2}", headers=_bearer(admin1)
        )
        assert r2.status_code == 204

        # secondary token now invalid
        r3 = await ac.get("/api/admin/tokens", headers=_bearer(admin2))
        assert r3.status_code == 401


@pytest.mark.asyncio
async def test_cannot_revoke_own_token(wired_app, migrated_db):
    import hashlib
    admin = await mint_admin_token(name="self")

    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        own_prefix = hashlib.sha256(admin.encode()).hexdigest()[:8]
        r = await ac.delete(
            f"/api/admin/tokens/{own_prefix}", headers=_bearer(admin)
        )
    assert r.status_code == 409
