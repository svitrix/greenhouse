from datetime import datetime, timezone

import pytest
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.models import Device
from app.repositories import commands as repo


async def _make_device(session, device_id: str) -> None:
    now = datetime.now(tz=timezone.utc)
    session.add(
        Device(
            device_id=device_id,
            profile_id="gh-coordinator-v1",
            first_seen_at=now,
            last_seen_at=now,
        )
    )
    await session.commit()


@pytest.mark.asyncio
async def test_enqueue_then_claim_flips_to_sent(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        await _make_device(s, "gh-cmd-flip")
        cmd = await repo.enqueue(
            s, device_id="gh-cmd-flip", command="pump_on",
            params={"duration_ms": 5000}, created_by="tg:tester",
        )
        assert cmd.status == "pending"

        pending = await repo.claim_pending(s, "gh-cmd-flip")
        assert [c.id for c in pending] == [cmd.id]
        assert pending[0].status == "sent"
        assert pending[0].claimed_at is not None

        # A second poll yields nothing — the command was already handed out.
        assert await repo.claim_pending(s, "gh-cmd-flip") == []


@pytest.mark.asyncio
async def test_enqueue_unknown_device_raises(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        with pytest.raises(repo.DeviceNotFound):
            await repo.enqueue(
                s, device_id="gh-cmd-absent", command="pump_off",
                params=None, created_by=None,
            )


@pytest.mark.asyncio
async def test_ack_is_scoped_to_owning_device(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        await _make_device(s, "gh-cmd-owner")
        await _make_device(s, "gh-cmd-other")
        cmd = await repo.enqueue(
            s, device_id="gh-cmd-owner", command="pump_on",
            params=None, created_by=None,
        )

        # A different device cannot ack someone else's command.
        assert await repo.ack(
            s, cmd.id, device_id="gh-cmd-other", status="acked", result=None
        ) is None

        acked = await repo.ack(
            s, cmd.id, device_id="gh-cmd-owner", status="acked",
            result={"ran_ms": 5000},
        )
        assert acked is not None
        assert acked.status == "acked"
        assert acked.acked_at is not None
        assert acked.result_json == {"ran_ms": 5000}
