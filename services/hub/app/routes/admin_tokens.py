from datetime import datetime

from fastapi import APIRouter, Depends, HTTPException, status
from pydantic import BaseModel
from sqlalchemy import delete, select
from sqlalchemy.ext.asyncio import AsyncSession

from app.auth_admin import AuthenticatedAdmin, verify_admin
from app.db import get_session
from app.models import AdminToken


router = APIRouter(prefix="/api/admin/tokens", tags=["admin_tokens"])


class AdminTokenOut(BaseModel):
    hash_prefix:  str
    name:         str | None
    created_at:   datetime
    last_used_at: datetime | None


@router.get("", response_model=list[AdminTokenOut])
async def list_admin_tokens(
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> list[AdminTokenOut]:
    rows = (
        await session.execute(
            select(AdminToken).order_by(AdminToken.created_at.desc())
        )
    ).scalars().all()
    return [
        AdminTokenOut(
            hash_prefix=r.token_hash[:8],
            name=r.name,
            created_at=r.created_at,
            last_used_at=r.last_used_at,
        )
        for r in rows
    ]


@router.delete("/{hash_prefix}", status_code=status.HTTP_204_NO_CONTENT)
async def revoke_admin_token(
    hash_prefix: str,
    admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> None:
    if not hash_prefix or len(hash_prefix) < 4:
        raise HTTPException(status_code=400, detail="hash_prefix too short")
    if admin.token_hash.startswith(hash_prefix):
        raise HTTPException(
            status_code=409,
            detail="cannot revoke the token used to make this request",
        )
    result = await session.execute(
        delete(AdminToken).where(AdminToken.token_hash.like(f"{hash_prefix}%"))
    )
    await session.commit()
    if (result.rowcount or 0) == 0:
        raise HTTPException(status_code=404, detail="no matching token")
