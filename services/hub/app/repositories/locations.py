from uuid import UUID, uuid4

from sqlalchemy import delete, func, select, update
from sqlalchemy.ext.asyncio import AsyncSession

from app.models import Location
from app.schemas.locations import LocationIn, LocationPatch


async def list_all(session: AsyncSession) -> list[Location]:
    return list(
        (await session.execute(select(Location).order_by(Location.name))).scalars().all()
    )


async def get_by_id(session: AsyncSession, location_id: UUID) -> Location | None:
    return (
        await session.execute(select(Location).where(Location.id == location_id))
    ).scalar_one_or_none()


async def create(session: AsyncSession, payload: LocationIn) -> Location:
    loc = Location(
        id=uuid4(),
        name=payload.name,
        address=payload.address,
        timezone=payload.timezone,
        notes=payload.notes,
        created_at=func.now(),
    )
    session.add(loc)
    await session.commit()
    await session.refresh(loc)
    return loc


async def patch(
    session: AsyncSession, location_id: UUID, payload: LocationPatch
) -> Location | None:
    updates = {k: v for k, v in payload.model_dump(exclude_unset=True).items()}
    if not updates:
        return await get_by_id(session, location_id)
    await session.execute(
        update(Location).where(Location.id == location_id).values(**updates)
    )
    await session.commit()
    return await get_by_id(session, location_id)


async def delete_by_id(session: AsyncSession, location_id: UUID) -> bool:
    result = await session.execute(delete(Location).where(Location.id == location_id))
    await session.commit()
    return (result.rowcount or 0) > 0
