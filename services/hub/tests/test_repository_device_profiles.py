import pytest
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.repositories.device_profiles import get_specs


@pytest.mark.asyncio
async def test_coordinator_v1_specs_returned(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        specs = await get_specs(s, "gh-coordinator-v1")
    assert specs is not None
    assert len(specs) == 6
    kinds = sorted({sp["kind"] for sp in specs})
    assert kinds == [
        "air_humidity", "air_temp", "battery_pct", "battery_v",
        "soil_moist", "soil_temp",
    ]


@pytest.mark.asyncio
async def test_unknown_profile_returns_none(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        specs = await get_specs(s, "gh-nonexistent-v9")
    assert specs is None
