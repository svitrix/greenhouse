import argparse
import asyncio
import hashlib
import secrets

from sqlalchemy import func, select
from sqlalchemy.dialects.postgresql import insert as pg_insert
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.db import make_engine
from app.models import AdminToken


async def mint_admin_token(name: str | None = None) -> str:
    token = secrets.token_hex(32)
    digest = hashlib.sha256(token.encode()).hexdigest()

    engine = make_engine()
    maker = async_sessionmaker(engine, expire_on_commit=False)
    try:
        async with maker() as s:
            if name is not None:
                existing = (
                    await s.execute(
                        select(AdminToken.token_hash).where(AdminToken.name == name)
                    )
                ).scalar_one_or_none()
                if existing:
                    raise ValueError(f"admin token named {name!r} already exists")

            await s.execute(
                pg_insert(AdminToken).values(
                    token_hash=digest,
                    name=name,
                    created_at=func.now(),
                )
            )
            await s.commit()
    finally:
        await engine.dispose()

    return token


def _main() -> None:
    parser = argparse.ArgumentParser(description="Mint a new admin API token.")
    parser.add_argument("--name", default=None,
                        help="Friendly identifier (browser-laptop, ml-pipeline, ...)")
    args = parser.parse_args()
    token = asyncio.run(mint_admin_token(args.name))
    print(f"admin_token: {token}")
    if args.name:
        print(f"name:        {args.name}")
    print("Store this token once - only the hash is persisted.")


if __name__ == "__main__":
    _main()
