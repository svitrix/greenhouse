from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.ext.asyncio import AsyncSession

from app.auth_admin import AuthenticatedAdmin, verify_admin
from app.db import get_session
from app.repositories import devices as repo
from app.schemas.devices import DeviceOut, DevicePatch


router = APIRouter(prefix="/api/devices", tags=["devices"])


@router.get("", response_model=list[DeviceOut])
async def list_devices(
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> list[DeviceOut]:
    return [DeviceOut(**row) for row in await repo.list_with_metrics(session)]


@router.get("/{device_id}", response_model=DeviceOut)
async def get_device(
    device_id: str,
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> DeviceOut:
    row = await repo.get_by_id(session, device_id)
    if row is None:
        raise HTTPException(status_code=404, detail="not found")
    return DeviceOut(**row)


@router.patch("/{device_id}", response_model=DeviceOut)
async def patch_device(
    device_id: str,
    body: DevicePatch,
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> DeviceOut:
    row = await repo.patch(session, device_id, body)
    if row is None:
        raise HTTPException(status_code=404, detail="not found")
    return DeviceOut(**row)


@router.post("/{device_id}/revoke", status_code=status.HTTP_204_NO_CONTENT)
async def revoke_device(
    device_id: str,
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> None:
    ok = await repo.revoke_credential(session, device_id)
    if not ok:
        raise HTTPException(status_code=404, detail="no credential to revoke")


@router.delete("/{device_id}", status_code=status.HTTP_204_NO_CONTENT)
async def delete_device(
    device_id: str,
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> None:
    ok = await repo.hard_delete(session, device_id)
    if not ok:
        raise HTTPException(status_code=404, detail="not found")
