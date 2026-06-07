import pytest
from sqlalchemy import text
from sqlalchemy.ext.asyncio import async_sessionmaker


@pytest.mark.asyncio
async def test_schema_tables_exist(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        result = await s.execute(
            text(
                "SELECT tablename FROM pg_tables WHERE schemaname='public' "
                "ORDER BY tablename"
            )
        )
        tables = [row[0] for row in result.all()]
    expected = {"devices", "sensors", "readings", "events", "device_credentials"}
    assert expected.issubset(set(tables)), f"missing: {expected - set(tables)}"


@pytest.mark.asyncio
async def test_readings_is_hypertable(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        result = await s.execute(
            text(
                "SELECT hypertable_name FROM timescaledb_information.hypertables "
                "WHERE hypertable_name IN ('readings','events') "
                "ORDER BY hypertable_name"
            )
        )
        names = [row[0] for row in result.all()]
    assert names == ["events", "readings"]
