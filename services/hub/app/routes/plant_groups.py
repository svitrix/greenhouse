from uuid import UUID

from fastapi import APIRouter, Depends, HTTPException, Query, status
from sqlalchemy.ext.asyncio import AsyncSession

from app.auth_admin import AuthenticatedAdmin, verify_admin
from app.db import get_session
from app.repositories import plant_groups as repo
from app.schemas.plant_groups import PlantGroupIn, PlantGroupOut, PlantGroupPatch


router = APIRouter(prefix="/api/plant_groups", tags=["plant_groups"])


@router.get("", response_model=list[PlantGroupOut])
async def list_groups(
    location_id: UUID | None = Query(default=None),
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> list[PlantGroupOut]:
    return [
        PlantGroupOut.model_validate(pg, from_attributes=True)
        for pg in await repo.list_filtered(session, location_id=location_id)
    ]


@router.post("", response_model=PlantGroupOut, status_code=status.HTTP_201_CREATED)
async def create_group(
    body: PlantGroupIn,
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> PlantGroupOut:
    pg = await repo.create(session, body)
    return PlantGroupOut.model_validate(pg, from_attributes=True)


@router.get("/{pg_id}", response_model=PlantGroupOut)
async def get_group(
    pg_id: UUID,
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> PlantGroupOut:
    pg = await repo.get_by_id(session, pg_id)
    if pg is None:
        raise HTTPException(status_code=404, detail="not found")
    return PlantGroupOut.model_validate(pg, from_attributes=True)


@router.patch("/{pg_id}", response_model=PlantGroupOut)
async def patch_group(
    pg_id: UUID,
    body: PlantGroupPatch,
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> PlantGroupOut:
    pg = await repo.patch(session, pg_id, body)
    if pg is None:
        raise HTTPException(status_code=404, detail="not found")
    return PlantGroupOut.model_validate(pg, from_attributes=True)


@router.delete("/{pg_id}", status_code=status.HTTP_204_NO_CONTENT)
async def delete_group(
    pg_id: UUID,
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> None:
    ok = await repo.delete_by_id(session, pg_id)
    if not ok:
        raise HTTPException(status_code=404, detail="not found")
