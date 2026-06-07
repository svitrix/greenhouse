import pytest
from sqlalchemy import text
from sqlalchemy.ext.asyncio import async_sessionmaker


@pytest.mark.asyncio
async def test_can_execute_select_1(db_session):
    result = await db_session.execute(text("SELECT 1"))
    assert result.scalar() == 1


@pytest.mark.asyncio
async def test_timescaledb_extension_present(db_engine):
    maker = async_sessionmaker(db_engine, expire_on_commit=False)
    async with maker() as s:
        result = await s.execute(
            text("SELECT extname FROM pg_extension WHERE extname = 'timescaledb'")
        )
        assert result.scalar() == "timescaledb"
