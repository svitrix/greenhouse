from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from app.models import DeviceProfile


async def get_specs(
    session: AsyncSession, profile_id: str,
) -> list[dict] | None:
    row = (
        await session.execute(
            select(DeviceProfile.sensor_specs).where(
                DeviceProfile.profile_id == profile_id
            )
        )
    ).scalar_one_or_none()
    return row
