import pytest
from sqlalchemy import text
from sqlalchemy.ext.asyncio import async_sessionmaker


@pytest.mark.asyncio
async def test_0006_creates_device_commands(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        cols = [
            r[0]
            for r in (
                await s.execute(
                    text(
                        "SELECT column_name FROM information_schema.columns "
                        "WHERE table_name='device_commands' ORDER BY column_name"
                    )
                )
            ).all()
        ]
    assert cols == [
        "acked_at", "claimed_at", "command", "created_at", "created_by",
        "device_id", "id", "params_json", "result_json", "status",
    ]


@pytest.mark.asyncio
async def test_0006_has_partial_pending_index(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        idx = (
            await s.execute(
                text(
                    "SELECT indexname FROM pg_indexes "
                    "WHERE tablename='device_commands' "
                    "AND indexname='ix_device_commands_pending'"
                )
            )
        ).scalar_one_or_none()
    assert idx == "ix_device_commands_pending"
