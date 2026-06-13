from datetime import datetime

from fastapi import APIRouter, Depends, Query
from sqlalchemy.ext.asyncio import AsyncSession

from app.auth_admin import AuthenticatedAdmin, verify_admin
from app.db import get_session
from app.repositories import events as repo
from app.schemas.events import EventOut


router = APIRouter(prefix="/api/events", tags=["events"])


@router.get("", response_model=list[EventOut])
async def list_events(
    kind: list[str] | None = Query(default=None),
    since: datetime | None = Query(default=None),
    limit: int = Query(default=50, ge=1, le=500),
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> list[EventOut]:
    rows = await repo.list_recent(session, kinds=kind, since=since, limit=limit)
    return [EventOut(**row) for row in rows]
