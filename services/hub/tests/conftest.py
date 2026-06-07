import os
import subprocess
import sys

import pytest
import pytest_asyncio
from sqlalchemy.ext.asyncio import async_sessionmaker, create_async_engine
from testcontainers.postgres import PostgresContainer


@pytest.fixture(scope="session")
def pg_container():
    with PostgresContainer("timescale/timescaledb:latest-pg16") as c:
        sync_url = c.get_connection_url()
        async_url = sync_url.replace("postgresql+psycopg2", "postgresql+asyncpg")
        os.environ["DB_URL"] = async_url
        yield async_url


@pytest_asyncio.fixture
async def db_engine(pg_container):
    engine = create_async_engine(pg_container)
    yield engine
    await engine.dispose()


@pytest_asyncio.fixture
async def db_session(db_engine):
    maker = async_sessionmaker(db_engine, expire_on_commit=False)
    async with maker() as s:
        yield s


@pytest_asyncio.fixture
async def migrated_db(pg_container, db_engine):
    """Runs `alembic upgrade head` against the test container before yielding.

    Uses `python -m alembic` instead of bare `alembic` so the venv's
    Python and its installed alembic are guaranteed to be on the call
    even when PATH doesn't include `.venv/bin/`.
    """
    env = os.environ.copy()
    env["DB_URL"] = pg_container
    backend_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    subprocess.run(
        [sys.executable, "-m", "alembic", "upgrade", "head"],
        cwd=backend_dir,
        env=env,
        check=True,
    )
    yield db_engine
