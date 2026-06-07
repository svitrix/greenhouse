from uuid import UUID

from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.ext.asyncio import AsyncSession

from app.auth_admin import AuthenticatedAdmin, verify_admin
from app.db import get_session
from app.repositories import locations as repo
from app.schemas.locations import LocationIn, LocationOut, LocationPatch


router = APIRouter(prefix="/api/locations", tags=["locations"])


@router.get("", response_model=list[LocationOut])
async def list_locations(
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> list[LocationOut]:
    return [LocationOut.model_validate(loc, from_attributes=True)
            for loc in await repo.list_all(session)]


@router.post("", response_model=LocationOut, status_code=status.HTTP_201_CREATED)
async def create_location(
    body: LocationIn,
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> LocationOut:
    loc = await repo.create(session, body)
    return LocationOut.model_validate(loc, from_attributes=True)


@router.get("/{location_id}", response_model=LocationOut)
async def get_location(
    location_id: UUID,
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> LocationOut:
    loc = await repo.get_by_id(session, location_id)
    if loc is None:
        raise HTTPException(status_code=404, detail="not found")
    return LocationOut.model_validate(loc, from_attributes=True)


@router.patch("/{location_id}", response_model=LocationOut)
async def patch_location(
    location_id: UUID,
    body: LocationPatch,
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> LocationOut:
    loc = await repo.patch(session, location_id, body)
    if loc is None:
        raise HTTPException(status_code=404, detail="not found")
    return LocationOut.model_validate(loc, from_attributes=True)


@router.delete("/{location_id}", status_code=status.HTTP_204_NO_CONTENT)
async def delete_location(
    location_id: UUID,
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> None:
    ok = await repo.delete_by_id(session, location_id)
    if not ok:
        raise HTTPException(status_code=404, detail="not found")
