import pytest
from sqlalchemy import select
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.models import Location, PlantGroup


@pytest.mark.asyncio
async def test_can_insert_location_and_plant_group(migrated_db):
    from datetime import datetime, timezone
    from uuid import uuid4

    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    loc_id = uuid4()
    pg_id  = uuid4()
    now = datetime.now(tz=timezone.utc)
    async with maker() as s:
        s.add(Location(
            id=loc_id, name="Дача", timezone="UTC", created_at=now,
        ))
        await s.flush()
        s.add(PlantGroup(
            id=pg_id, location_id=loc_id, name="Помидоры ряд 1",
            created_at=now,
        ))
        await s.commit()

    async with maker() as s:
        rows = (
            await s.execute(
                select(PlantGroup).where(PlantGroup.location_id == loc_id)
            )
        ).scalars().all()
    assert len(rows) == 1
    assert rows[0].name == "Помидоры ряд 1"
