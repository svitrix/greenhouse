import argon2
import pytest
from sqlalchemy import select
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.models import AdminUser
from app.tools.mint_admin_user import mint_admin_user


@pytest.mark.asyncio
async def test_mint_inserts_user_with_argon2_hash(migrated_db):
    await mint_admin_user("alice", "alicepass1234")
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        row = (
            await s.execute(select(AdminUser).where(AdminUser.username == "alice"))
        ).scalar_one()
    # argon2 hash starts with "$argon2"
    assert row.password_hash.startswith("$argon2")
    # and round-trips through verify
    argon2.PasswordHasher().verify(row.password_hash, "alicepass1234")


@pytest.mark.asyncio
async def test_mint_overwrites_password_on_duplicate(migrated_db):
    await mint_admin_user("bob", "oldpassword12")
    await mint_admin_user("bob", "newpassword34")
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        row = (
            await s.execute(select(AdminUser).where(AdminUser.username == "bob"))
        ).scalar_one()
    argon2.PasswordHasher().verify(row.password_hash, "newpassword34")
    with pytest.raises(argon2.exceptions.VerifyMismatchError):
        argon2.PasswordHasher().verify(row.password_hash, "oldpassword12")
