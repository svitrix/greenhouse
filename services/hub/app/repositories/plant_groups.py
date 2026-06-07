from uuid import UUID, uuid4

from sqlalchemy import delete, func, select, update
from sqlalchemy.ext.asyncio import AsyncSession

from app.models import PlantGroup
from app.schemas.plant_groups import PlantGroupIn, PlantGroupPatch


async def list_filtered(
    session: AsyncSession, *, location_id: UUID | None = None,
) -> list[PlantGroup]:
    stmt = select(PlantGroup).order_by(PlantGroup.name)
    if location_id is not None:
        stmt = stmt.where(PlantGroup.location_id == location_id)
    return list((await session.execute(stmt)).scalars().all())


async def get_by_id(session: AsyncSession, pg_id: UUID) -> PlantGroup | None:
    return (
        await session.execute(select(PlantGroup).where(PlantGroup.id == pg_id))
    ).scalar_one_or_none()


async def create(session: AsyncSession, payload: PlantGroupIn) -> PlantGroup:
    pg = PlantGroup(
        id=uuid4(),
        location_id=payload.location_id,
        name=payload.name,
        species=payload.species,
        planted_at=payload.planted_at,
        notes=payload.notes,
        created_at=func.now(),
    )
    session.add(pg)
    await session.commit()
    await session.refresh(pg)
    return pg


async def patch(
    session: AsyncSession, pg_id: UUID, payload: PlantGroupPatch,
) -> PlantGroup | None:
    updates = {k: v for k, v in payload.model_dump(exclude_unset=True).items()}
    if not updates:
        return await get_by_id(session, pg_id)
    await session.execute(
        update(PlantGroup).where(PlantGroup.id == pg_id).values(**updates)
    )
    await session.commit()
    return await get_by_id(session, pg_id)


async def delete_by_id(session: AsyncSession, pg_id: UUID) -> bool:
    result = await session.execute(delete(PlantGroup).where(PlantGroup.id == pg_id))
    await session.commit()
    return (result.rowcount or 0) > 0
