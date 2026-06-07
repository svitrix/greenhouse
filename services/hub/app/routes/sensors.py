from uuid import UUID

from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.ext.asyncio import AsyncSession

from app.auth_admin import AuthenticatedAdmin, verify_admin
from app.db import get_session
from app.repositories import sensors as repo
from app.schemas.sensors import SensorOut, SensorPatch


router = APIRouter(prefix="/api/sensors", tags=["sensors"])


@router.get("", response_model=list[SensorOut])
async def list_sensors(
    device_id: str | None = Query(default=None),
    plant_group_id: UUID | None = Query(default=None),
    kind: str | None = Query(default=None),
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> list[SensorOut]:
    return [
        SensorOut.model_validate(s, from_attributes=True)
        for s in await repo.list_filtered(
            session, device_id=device_id, plant_group_id=plant_group_id, kind=kind,
        )
    ]


@router.get("/{device_id}/{channel_id}/{kind}", response_model=SensorOut)
async def get_sensor(
    device_id: str,
    channel_id: int,
    kind: str,
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> SensorOut:
    sensor = await repo.get(session, device_id, channel_id, kind)
    if sensor is None:
        raise HTTPException(status_code=404, detail="not found")
    return SensorOut.model_validate(sensor, from_attributes=True)


@router.patch("/{device_id}/{channel_id}/{kind}", response_model=SensorOut)
async def patch_sensor(
    device_id: str,
    channel_id: int,
    kind: str,
    body: SensorPatch,
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> SensorOut:
    sensor = await repo.patch(session, device_id, channel_id, kind, body)
    if sensor is None:
        raise HTTPException(status_code=404, detail="not found")
    return SensorOut.model_validate(sensor, from_attributes=True)
