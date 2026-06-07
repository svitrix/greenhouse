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
async def test_pg_crud_and_filter_by_location(wired_app, migrated_db):
    admin = await mint_admin_token(name="pg-test")

    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        # create two locations + groups
        r_loc1 = await ac.post(
            "/api/locations", json={"name": "L1"}, headers=_bearer(admin)
        )
        r_loc2 = await ac.post(
            "/api/locations", json={"name": "L2"}, headers=_bearer(admin)
        )
        loc1, loc2 = r_loc1.json()["id"], r_loc2.json()["id"]

        await ac.post(
            "/api/plant_groups",
            json={"location_id": loc1, "name": "Помидоры"},
            headers=_bearer(admin),
        )
        await ac.post(
            "/api/plant_groups",
            json={"location_id": loc2, "name": "Огурцы"},
            headers=_bearer(admin),
        )

        # filter by location
        r = await ac.get(
            f"/api/plant_groups?location_id={loc1}", headers=_bearer(admin)
        )
        assert [pg["name"] for pg in r.json()] == ["Помидоры"]

        # cascade delete via location
        await ac.delete(f"/api/locations/{loc1}", headers=_bearer(admin))
        r2 = await ac.get(
            f"/api/plant_groups?location_id={loc1}", headers=_bearer(admin)
        )
        assert r2.json() == []
