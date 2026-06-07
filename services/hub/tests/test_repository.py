import pytest
from sqlalchemy import text
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.repositories.ingest import BatchInsertResult, insert_batch
from app.schemas.ingest import IngestBatch


def _batch(device_id="gh-a", n=3, kind="air_temp", ts0=1_700_000_000_000):
    return IngestBatch.model_validate({
        "device_id": device_id,
        "fw_version": "1.0.0",
        "batch_id": "01H",
        "readings": [
            {
                "ts": ts0 + i * 1000,
                "channel_id": 0,
                "kind": kind,
                "value": 20.0 + i,
                "status": 0,
            }
            for i in range(n)
        ],
        "events": [],
    })


@pytest.mark.asyncio
async def test_insert_creates_device_and_readings(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        res: BatchInsertResult = await insert_batch(s, _batch(n=3))
        await s.commit()
    assert res.accepted_readings == 3
    assert res.duplicates_skipped == 0

    async with maker() as s:
        count = (
            await s.execute(
                text("SELECT count(*) FROM readings WHERE device_id='gh-a'")
            )
        ).scalar()
        assert count == 3
        present = (
            await s.execute(text("SELECT device_id FROM devices WHERE device_id='gh-a'"))
        ).scalar()
        assert present == "gh-a"


@pytest.mark.asyncio
async def test_insert_dedups_on_natural_key(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        await insert_batch(s, _batch(n=3))
        await s.commit()
    async with maker() as s:
        res = await insert_batch(s, _batch(n=3))
        await s.commit()
    assert res.accepted_readings == 0
    assert res.duplicates_skipped == 3


@pytest.mark.asyncio
async def test_insert_events_recorded(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    payload = _batch(n=1).model_dump()
    payload["events"] = [
        {"ts": 1_700_000_500_000, "kind": "watered", "payload": {"ms": 8000}}
    ]
    batch = IngestBatch.model_validate(payload)
    async with maker() as s:
        res = await insert_batch(s, batch)
        await s.commit()
    assert res.accepted_events == 1
    async with maker() as s:
        count = (
            await s.execute(text("SELECT count(*) FROM events WHERE device_id='gh-a'"))
        ).scalar()
        assert count == 1
