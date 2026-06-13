from datetime import datetime, timedelta, timezone

from sqlalchemy import func, select, update
from sqlalchemy.ext.asyncio import AsyncSession
from ulid import ULID

from app.models import Device, DeviceCommand


class DeviceNotFound(Exception):
    """Raised when a command targets a device that is not registered."""


# Pending commands that no coordinator has claimed within this window are
# swept to 'expired' so a device that was offline does not flush a fistful
# of stale pump commands the moment it reconnects.
_PENDING_TTL = timedelta(minutes=15)


async def enqueue(
    session: AsyncSession,
    *,
    device_id: str,
    command: str,
    params: dict | None,
    created_by: str | None,
) -> DeviceCommand:
    exists = await session.execute(
        select(Device.device_id).where(Device.device_id == device_id)
    )
    if exists.scalar_one_or_none() is None:
        raise DeviceNotFound(device_id)

    row = DeviceCommand(
        id=str(ULID()),
        device_id=device_id,
        command=command,
        params_json=params,
        status="pending",
        created_by=created_by,
        created_at=datetime.now(tz=timezone.utc),
    )
    session.add(row)
    await session.commit()
    await session.refresh(row)
    return row


async def list_for_device(
    session: AsyncSession, device_id: str, *, limit: int = 50,
) -> list[DeviceCommand]:
    stmt = (
        select(DeviceCommand)
        .where(DeviceCommand.device_id == device_id)
        .order_by(DeviceCommand.created_at.desc())
        .limit(limit)
    )
    return list((await session.execute(stmt)).scalars().all())


async def claim_pending(
    session: AsyncSession, device_id: str, *, limit: int = 10,
) -> list[DeviceCommand]:
    """Return the device's pending commands and flip them to 'sent' in the
    same transaction, so a redelivery (firmware retry) does not double-fire
    the pump. Stale pending rows are expired first."""
    now = datetime.now(tz=timezone.utc)
    await session.execute(
        update(DeviceCommand)
        .where(
            DeviceCommand.device_id == device_id,
            DeviceCommand.status == "pending",
            DeviceCommand.created_at < now - _PENDING_TTL,
        )
        .values(status="expired")
    )

    stmt = (
        select(DeviceCommand)
        .where(
            DeviceCommand.device_id == device_id,
            DeviceCommand.status == "pending",
        )
        .order_by(DeviceCommand.created_at)
        .limit(limit)
        .with_for_update(skip_locked=True)
    )
    rows = list((await session.execute(stmt)).scalars().all())
    for row in rows:
        row.status = "sent"
        row.claimed_at = now
    await session.commit()
    return rows


async def ack(
    session: AsyncSession,
    command_id: str,
    *,
    device_id: str,
    status: str,
    result: dict | None,
) -> DeviceCommand | None:
    row = (
        await session.execute(
            select(DeviceCommand).where(
                DeviceCommand.id == command_id,
                DeviceCommand.device_id == device_id,
            )
        )
    ).scalar_one_or_none()
    if row is None:
        return None
    row.status = status
    row.result_json = result
    row.acked_at = func.now()
    await session.commit()
    await session.refresh(row)
    return row
