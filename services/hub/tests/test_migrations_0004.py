import pytest
from sqlalchemy import text
from sqlalchemy.ext.asyncio import async_sessionmaker


@pytest.mark.asyncio
async def test_0004_seeds_coordinator_v1_profile(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        result = await s.execute(
            text(
                "SELECT name, manufacturer, jsonb_array_length(sensor_specs) "
                "FROM device_profiles WHERE profile_id = 'gh-coordinator-v1'"
            )
        )
        row = result.first()
    assert row is not None
    name, manuf, count = row
    assert name == "Greenhouse Coordinator v1"
    assert manuf == "svitrix"
    assert count == 6


@pytest.mark.asyncio
async def test_0004_devices_profile_id_not_null(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        nullable = (
            await s.execute(
                text(
                    "SELECT is_nullable FROM information_schema.columns "
                    "WHERE table_name='devices' AND column_name='profile_id'"
                )
            )
        ).scalar_one()
    assert nullable == "NO"
