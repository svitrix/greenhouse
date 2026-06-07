from uuid import UUID

from sqlalchemy import select, update
from sqlalchemy.ext.asyncio import AsyncSession

from app.models import Sensor
from app.schemas.sensors import SensorPatch


async def list_filtered(
    session: AsyncSession,
    *,
    device_id: str | None = None,
    plant_group_id: UUID | None = None,
    kind: str | None = None,
) -> list[Sensor]:
    stmt = select(Sensor).order_by(Sensor.device_id, Sensor.channel_id, Sensor.kind)
    if device_id is not None:
        stmt = stmt.where(Sensor.device_id == device_id)
    if plant_group_id is not None:
        stmt = stmt.where(Sensor.plant_group_id == plant_group_id)
    if kind is not None:
        stmt = stmt.where(Sensor.kind == kind)
    return list((await session.execute(stmt)).scalars().all())


async def get(
    session: AsyncSession, device_id: str, channel_id: int, kind: str,
) -> Sensor | None:
    return (
        await session.execute(
            select(Sensor).where(
                Sensor.device_id == device_id,
                Sensor.channel_id == channel_id,
                Sensor.kind == kind,
            )
        )
    ).scalar_one_or_none()


async def patch(
    session: AsyncSession,
    device_id: str, channel_id: int, kind: str,
    payload: SensorPatch,
) -> Sensor | None:
    updates = {k: v for k, v in payload.model_dump(exclude_unset=True).items()}
    if not updates:
        return await get(session, device_id, channel_id, kind)
    await session.execute(
        update(Sensor).where(
            Sensor.device_id == device_id,
            Sensor.channel_id == channel_id,
            Sensor.kind == kind,
        ).values(**updates)
    )
    await session.commit()
    return await get(session, device_id, channel_id, kind)
