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


def _bearer(t: str) -> dict[str, str]:
    return {"Authorization": f"Bearer {t}"}


@pytest.mark.asyncio
async def test_crud_full_cycle(wired_app, migrated_db):
    admin = await mint_admin_token(name="loc-test")

    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        # create
        r = await ac.post(
            "/api/locations",
            json={"name": "Дача", "timezone": "Europe/Kyiv"},
            headers=_bearer(admin),
        )
        assert r.status_code == 201, r.text
        loc_id = r.json()["id"]

        # get one
        r2 = await ac.get(f"/api/locations/{loc_id}", headers=_bearer(admin))
        assert r2.status_code == 200
        assert r2.json()["name"] == "Дача"

        # list
        r3 = await ac.get("/api/locations", headers=_bearer(admin))
        assert any(x["id"] == loc_id for x in r3.json())

        # patch
        r4 = await ac.patch(
            f"/api/locations/{loc_id}",
            json={"name": "Дача (обновлённая)"},
            headers=_bearer(admin),
        )
        assert r4.status_code == 200
        assert r4.json()["name"] == "Дача (обновлённая)"
        assert r4.json()["timezone"] == "Europe/Kyiv"  # untouched

        # delete
        r5 = await ac.delete(f"/api/locations/{loc_id}", headers=_bearer(admin))
        assert r5.status_code == 204

        # gone
        r6 = await ac.get(f"/api/locations/{loc_id}", headers=_bearer(admin))
        assert r6.status_code == 404


@pytest.mark.asyncio
async def test_list_requires_admin(wired_app):
    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.get("/api/locations")
    assert r.status_code == 401
