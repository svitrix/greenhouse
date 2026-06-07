from collections.abc import AsyncIterator

from sqlalchemy.ext.asyncio import (
    AsyncEngine,
    AsyncSession,
    async_sessionmaker,
    create_async_engine,
)

from app.config import get_settings


def make_engine(url: str | None = None) -> AsyncEngine:
    return create_async_engine(
        url or get_settings().DB_URL,
        pool_pre_ping=True,
        pool_size=5,
        max_overflow=5,
    )


_engine: AsyncEngine | None = None
_sessionmaker: async_sessionmaker[AsyncSession] | None = None


def init_engine(url: str | None = None) -> None:
    global _engine, _sessionmaker
    _engine = make_engine(url)
    _sessionmaker = async_sessionmaker(_engine, expire_on_commit=False)


async def get_session() -> AsyncIterator[AsyncSession]:
    assert _sessionmaker is not None, "init_engine() not called"
    async with _sessionmaker() as s:
        yield s
