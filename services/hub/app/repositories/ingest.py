from dataclasses import dataclass
from datetime import datetime, timezone

from sqlalchemy import func, select, tuple_, update
from sqlalchemy.dialects.postgresql import insert as pg_insert
from sqlalchemy.ext.asyncio import AsyncSession

from app.models import Device, Event, Reading, Sensor
from app.schemas.ingest import IngestBatch


@dataclass(frozen=True)
class BatchInsertResult:
    accepted_readings: int
    accepted_events: int
    duplicates_skipped: int


class UnknownSensorForDevice(Exception):
    """Raised when /ingest carries a (channel_id, kind) triplet that
    the device's profile didn't declare. Surfaces as 400 at the route
    layer; the firmware contract violation should be loud."""

    def __init__(self, *, device_id: str, triplets: list[tuple[int, str]]) -> None:
        self.device_id = device_id
        self.triplets = triplets
        super().__init__(
            f"device {device_id} reported unknown sensor triplets: {triplets}"
        )


def _ts_to_dt(ts_ms: int) -> datetime:
    return datetime.fromtimestamp(ts_ms / 1000.0, tz=timezone.utc)


_FALLBACK_PROFILE_ID = "gh-coordinator-v1"


async def _upsert_device(
    session: AsyncSession, device_id: str, fw_version: str
) -> None:
    # Postgres evaluates NOT NULL on the prospective INSERT row before the
    # ON CONFLICT arbiter check fires, so we must supply *some* valid
    # profile_id even for already-paired devices (where DO UPDATE will
    # trigger). The DO UPDATE clause deliberately excludes profile_id,
    # so the row's true value — set at pairing time — is preserved.
    # For the legacy env-map flow (DEVICE_API_KEYS) the device row is
    # created here with the seed profile.
    stmt = pg_insert(Device).values(
        device_id=device_id,
        fw_version=fw_version,
        profile_id=_FALLBACK_PROFILE_ID,
        first_seen_at=func.now(),
        last_seen_at=func.now(),
    )
    stmt = stmt.on_conflict_do_update(
        index_elements=[Device.device_id],
        set_={
            "fw_version": stmt.excluded.fw_version,
            "last_seen_at": func.now(),
        },
    )
    await session.execute(stmt)


async def insert_batch(session: AsyncSession, batch: IngestBatch) -> BatchInsertResult:
    await _upsert_device(session, batch.device_id, batch.fw_version)

    # Validate that all (channel_id, kind) tuples in the batch correspond
    # to sensors registered for this device — i.e. declared by its
    # profile at pairing time. Unknown triplets are rejected; this
    # preserves the hub's "knows about devices" property (spec §6).
    triplets = {(r.channel_id, r.kind) for r in batch.readings}
    if triplets:
        existing = (
            await session.execute(
                select(Sensor.channel_id, Sensor.kind).where(
                    Sensor.device_id == batch.device_id,
                    tuple_(Sensor.channel_id, Sensor.kind).in_(list(triplets)),
                )
            )
        ).all()
        existing_set = {(row[0], row[1]) for row in existing}
        unknown = triplets - existing_set
        if unknown:
            raise UnknownSensorForDevice(
                device_id=batch.device_id, triplets=sorted(unknown),
            )

    reading_rows = [
        {
            "ts": _ts_to_dt(r.ts),
            "device_id": batch.device_id,
            "channel_id": r.channel_id,
            "kind": r.kind,
            "value": r.value,
            "raw": r.raw,
            "status": r.status,
        }
        for r in batch.readings
    ]
    submitted = len(reading_rows)
    if submitted == 0:
        accepted = 0
    else:
        stmt = pg_insert(Reading).values(reading_rows).on_conflict_do_nothing(
            index_elements=[
                Reading.device_id, Reading.channel_id, Reading.kind, Reading.ts,
            ]
        )
        result = await session.execute(stmt)
        accepted = result.rowcount or 0

        # Denormalised hot cache: update sensors.last_value + last_value_at
        # for each unique (channel_id, kind) triplet in this batch, using
        # the max-ts row per triplet (spec §2). One UPDATE per unique
        # sensor regardless of how many readings the batch contained.
        max_ts: dict[tuple[int, str], tuple[datetime, float]] = {}
        for r in reading_rows:
            key = (r["channel_id"], r["kind"])
            ts = r["ts"]
            if key not in max_ts or ts > max_ts[key][0]:
                max_ts[key] = (ts, r["value"])
        for (cid, kind), (ts_val, val) in max_ts.items():
            await session.execute(
                update(Sensor)
                .where(
                    Sensor.device_id == batch.device_id,
                    Sensor.channel_id == cid,
                    Sensor.kind == kind,
                )
                .values(last_value=val, last_value_at=ts_val)
            )
    duplicates = submitted - accepted

    accepted_events = 0
    if batch.events:
        event_rows = [
            {
                "ts": _ts_to_dt(e.ts),
                "device_id": batch.device_id,
                "kind": e.kind,
                "payload_json": e.payload,  # JSONB column accepts dict directly
            }
            for e in batch.events
        ]
        result = await session.execute(pg_insert(Event).values(event_rows))
        accepted_events = result.rowcount or len(event_rows)

    return BatchInsertResult(
        accepted_readings=accepted,
        accepted_events=accepted_events,
        duplicates_skipped=duplicates,
    )
