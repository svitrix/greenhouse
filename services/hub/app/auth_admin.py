import hashlib
from dataclasses import dataclass

from fastapi import BackgroundTasks, Depends, Header, HTTPException, status
from sqlalchemy import func, select, update
from sqlalchemy.ext.asyncio import AsyncSession

from app import db as _db
from app.db import get_session
from app.models import AdminToken


@dataclass(frozen=True)
class AuthenticatedAdmin:
    token_hash: str
    name: str | None


def _extract_bearer(authorization: str | None) -> str:
    if not authorization or not authorization.lower().startswith("bearer "):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED, detail="missing bearer"
        )
    return authorization.split(" ", 1)[1].strip()


async def _touch_last_used(token_hash: str) -> None:
    """Runs in a BackgroundTask context — opens its own session so it
    doesn't share the request's already-closed transaction."""
    if _db._sessionmaker is None:
        return
    async with _db._sessionmaker() as s:
        await s.execute(
            update(AdminToken)
            .where(AdminToken.token_hash == token_hash)
            .values(last_used_at=func.now())
        )
        await s.commit()


async def verify_admin(
    background: BackgroundTasks,
    authorization: str | None = Header(default=None),
    session: AsyncSession = Depends(get_session),
) -> AuthenticatedAdmin:
    token = _extract_bearer(authorization)
    digest = hashlib.sha256(token.encode("utf-8")).hexdigest()

    row = (
        await session.execute(
            select(AdminToken.token_hash, AdminToken.name).where(
                AdminToken.token_hash == digest
            )
        )
    ).first()
    if row is None:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED, detail="invalid admin token"
        )

    background.add_task(_touch_last_used, digest)
    return AuthenticatedAdmin(token_hash=row[0], name=row[1])
