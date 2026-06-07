import pytest
from sqlalchemy import text
from sqlalchemy.ext.asyncio import async_sessionmaker


@pytest.mark.asyncio
async def test_0003_adds_new_tables(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        result = await s.execute(
            text(
                "SELECT tablename FROM pg_tables "
                "WHERE schemaname='public' AND tablename IN "
                "('locations', 'plant_groups') ORDER BY tablename"
            )
        )
        names = [row[0] for row in result.all()]
    assert names == ["locations", "plant_groups"]


@pytest.mark.asyncio
async def test_0003_devices_has_friendly_name_and_location_id(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        cols = (
            await s.execute(
                text(
                    "SELECT column_name FROM information_schema.columns "
                    "WHERE table_name='devices' "
                    "AND column_name IN ('friendly_name','location_id') "
                    "ORDER BY column_name"
                )
            )
        ).scalars().all()
    assert cols == ["friendly_name", "location_id"]


@pytest.mark.asyncio
async def test_0003_sensors_extended(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        cols = sorted(
            (
                await s.execute(
                    text(
                        "SELECT column_name FROM information_schema.columns "
                        "WHERE table_name='sensors' "
                        "AND column_name IN ("
                        "'friendly_name','plant_group_id','calibration_json',"
                        "'created_at','last_value','last_value_at')"
                    )
                )
            ).scalars().all()
        )
    assert cols == [
        "calibration_json", "created_at", "friendly_name",
        "last_value", "last_value_at", "plant_group_id",
    ]
