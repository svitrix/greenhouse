import argparse
import asyncio

import argon2
from sqlalchemy import func
from sqlalchemy.dialects.postgresql import insert as pg_insert
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.db import make_engine
from app.models import AdminUser


async def mint_admin_user(username: str, password: str) -> None:
    hasher = argon2.PasswordHasher()
    pw_hash = hasher.hash(password)
    engine = make_engine()
    maker = async_sessionmaker(engine, expire_on_commit=False)
    try:
        async with maker() as s:
            stmt = pg_insert(AdminUser).values(
                username=username,
                password_hash=pw_hash,
                created_at=func.now(),
            )
            stmt = stmt.on_conflict_do_update(
                index_elements=[AdminUser.username],
                set_={"password_hash": pw_hash},
            )
            await s.execute(stmt)
            await s.commit()
    finally:
        await engine.dispose()


def _main() -> None:
    parser = argparse.ArgumentParser(description="Create or update an admin user.")
    parser.add_argument("--username", required=True)
    parser.add_argument("--password", required=True)
    args = parser.parse_args()
    asyncio.run(mint_admin_user(args.username, args.password))
    print(f"admin user {args.username!r} created/updated.")


if __name__ == "__main__":
    _main()
