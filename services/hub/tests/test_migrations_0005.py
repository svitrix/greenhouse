import pytest
from sqlalchemy import text
from sqlalchemy.ext.asyncio import async_sessionmaker


@pytest.mark.asyncio
async def test_0005_creates_admin_users(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        result = await s.execute(
            text(
                "SELECT column_name, is_nullable FROM information_schema.columns "
                "WHERE table_name='admin_users' ORDER BY column_name"
            )
        )
        rows = [(r[0], r[1]) for r in result.all()]
    assert rows == [
        ("created_at",    "NO"),
        ("password_hash", "NO"),
        ("username",      "NO"),
    ]


@pytest.mark.asyncio
async def test_0005_username_is_primary_key(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        pk = (
            await s.execute(
                text(
                    "SELECT column_name FROM information_schema.key_column_usage "
                    "WHERE table_name='admin_users' "
                    "AND constraint_name LIKE 'admin_users_pkey%'"
                )
            )
        ).scalar_one()
    assert pk == "username"
