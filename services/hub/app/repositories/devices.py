from datetime import datetime, timedelta, timezone

from sqlalchemy import delete, func, select, text, update
from sqlalchemy.ext.asyncio import AsyncSession

from app.models import Device, DeviceCredential, Sensor
from app.schemas.devices import DevicePatch


_ONLINE_WINDOW = timedelta(minutes=30)


async def list_with_metrics(session: AsyncSession) -> list[dict]:
    stmt = (
        select(
            Device.device_id, Device.friendly_name, Device.fw_version,
            Device.profile_id, Device.location_id, Device.last_seen_at,
            func.count(Sensor.device_id).label("sensors_count"),
        )
        .outerjoin(Sensor, Sensor.device_id == Device.device_id)
        .group_by(
            Device.device_id, Device.friendly_name, Device.fw_version,
            Device.profile_id, Device.location_id, Device.last_seen_at,
        )
        .order_by(Device.device_id)
    )
    rows = (await session.execute(stmt)).all()
    now = datetime.now(tz=timezone.utc)
    result = []
    for r in rows:
        result.append({
            "device_id":     r.device_id,
            "friendly_name": r.friendly_name,
            "fw_version":    r.fw_version,
            "profile_id":    r.profile_id,
            "location_id":   r.location_id,
            "last_seen_at":  r.last_seen_at,
            "sensors_count": int(r.sensors_count),
            "online":        (r.last_seen_at is not None
                              and r.last_seen_at > now - _ONLINE_WINDOW),
        })
    return result


async def get_by_id(session: AsyncSession, device_id: str) -> dict | None:
    rows = await list_with_metrics(session)
    for row in rows:
        if row["device_id"] == device_id:
            return row
    return None


async def patch(
    session: AsyncSession, device_id: str, payload: DevicePatch,
) -> dict | None:
    updates = {k: v for k, v in payload.model_dump(exclude_unset=True).items()}
    if not updates:
        return await get_by_id(session, device_id)
    await session.execute(
        update(Device).where(Device.device_id == device_id).values(**updates)
    )
    await session.commit()
    return await get_by_id(session, device_id)


async def revoke_credential(session: AsyncSession, device_id: str) -> bool:
    result = await session.execute(
        delete(DeviceCredential).where(DeviceCredential.device_id == device_id)
    )
    await session.commit()
    return (result.rowcount or 0) > 0


async def hard_delete(session: AsyncSession, device_id: str) -> bool:
    # CASCADE handles credentials + sensors; readings/events are NOT FK'd,
    # so we manually delete those too.
    await session.execute(
        text("DELETE FROM readings WHERE device_id = :id"), {"id": device_id}
    )
    await session.execute(
        text("DELETE FROM events   WHERE device_id = :id"), {"id": device_id}
    )
    result = await session.execute(
        delete(Device).where(Device.device_id == device_id)
    )
    await session.commit()
    return (result.rowcount or 0) > 0
