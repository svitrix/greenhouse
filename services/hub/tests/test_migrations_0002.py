import pytest
from sqlalchemy import text
from sqlalchemy.ext.asyncio import async_sessionmaker


@pytest.mark.asyncio
async def test_0002_creates_admin_tokens_and_pairing_windows(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        result = await s.execute(
            text(
                "SELECT tablename FROM pg_tables "
                "WHERE schemaname='public' AND tablename IN "
                "('admin_tokens', 'pairing_windows') "
                "ORDER BY tablename"
            )
        )
        names = [row[0] for row in result.all()]
    assert names == ["admin_tokens", "pairing_windows"]


@pytest.mark.asyncio
async def test_pairing_windows_partial_index_exists(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        result = await s.execute(
            text(
                "SELECT indexdef FROM pg_indexes "
                "WHERE indexname = 'pairing_windows_expires_idx'"
            )
        )
        defn = result.scalar_one()
    # partial index condition is part of the definition
    assert "consumed_by IS NULL" in defn
