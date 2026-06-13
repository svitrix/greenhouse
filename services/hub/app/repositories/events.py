from datetime import datetime

from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from app.models import Device, Event


async def list_recent(
    session: AsyncSession,
    *,
    kinds: list[str] | None = None,
    since: datetime | None = None,
    limit: int = 50,
) -> list[dict]:
    """Append-only event feed, newest first. `since` is exclusive so a
    poller can pass its last-seen ts as a cursor without re-reading the
    boundary row."""
    stmt = (
        select(
            Event.ts, Event.device_id, Event.kind, Event.payload_json,
            Device.friendly_name,
        )
        .outerjoin(Device, Device.device_id == Event.device_id)
        .order_by(Event.ts.desc())
        .limit(limit)
    )
    if kinds:
        stmt = stmt.where(Event.kind.in_(kinds))
    if since is not None:
        stmt = stmt.where(Event.ts > since)

    rows = (await session.execute(stmt)).all()
    return [
        {
            "ts":          r.ts,
            "device_id":   r.device_id,
            "device_name": r.friendly_name,
            "kind":        r.kind,
            "payload":     r.payload_json,
        }
        for r in rows
    ]
