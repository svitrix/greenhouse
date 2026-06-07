import hashlib

import pytest
from sqlalchemy import select
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.models import AdminToken
from app.tools.mint_admin_token import mint_admin_token


@pytest.mark.asyncio
async def test_mint_inserts_row_with_correct_hash(migrated_db):
    token = await mint_admin_token(name="test-admin")
    assert len(token) == 64

    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        row = (
            await s.execute(
                select(AdminToken).where(AdminToken.name == "test-admin")
            )
        ).scalar_one()
    assert row.token_hash == hashlib.sha256(token.encode()).hexdigest()


@pytest.mark.asyncio
async def test_duplicate_name_raises(migrated_db):
    await mint_admin_token(name="dup")
    with pytest.raises(ValueError, match="already exists"):
        await mint_admin_token(name="dup")


@pytest.mark.asyncio
async def test_no_name_is_allowed(migrated_db):
    token1 = await mint_admin_token(name=None)
    token2 = await mint_admin_token(name=None)
    assert token1 != token2  # two anonymous tokens
