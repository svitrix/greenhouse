import hashlib
import secrets
from dataclasses import dataclass
from datetime import datetime, timedelta, timezone

from sqlalchemy import func, select, text
from sqlalchemy.dialects.postgresql import insert as pg_insert
from sqlalchemy.ext.asyncio import AsyncSession

from app.models import Device, DeviceCredential, PairingWindow, Sensor


def _gen_code() -> str:
    """6-digit numeric, leading zeros preserved."""
    return f"{secrets.randbelow(1_000_000):06d}"


@dataclass(frozen=True)
class OpenedWindow:
    code: str
    expires_at: datetime


@dataclass(frozen=True)
class ClaimResult:
    api_key: str
    device_id: str


class PairingError(Exception):
    """Base for pairing errors carrying an HTTP status code."""
    http_status: int = 400


class WindowNotAvailable(PairingError):
    """Code expired, used, or unknown."""
    http_status = 410


class DeviceAlreadyRegistered(PairingError):
    """Device already has a credential; revoke first."""
    http_status = 409


async def open_window(session: AsyncSession, ttl_seconds: int) -> OpenedWindow:
    # Lazy cleanup (spec §3): drop ancient rows in the same transaction.
    await session.execute(
        text(
            "DELETE FROM pairing_windows "
            "WHERE expires_at < now() - interval '24 hours' "
            "   OR (consumed_by IS NOT NULL AND consumed_at < now() - interval '7 days')"
        )
    )

    # Retry on the unlikely 6-digit collision; cap attempts.
    for _ in range(8):
        code = _gen_code()
        expires_at = datetime.now(tz=timezone.utc) + timedelta(seconds=ttl_seconds)
        try:
            await session.execute(
                pg_insert(PairingWindow).values(
                    code=code,
                    opens_at=func.now(),
                    expires_at=expires_at,
                )
            )
            await session.commit()
            return OpenedWindow(code=code, expires_at=expires_at)
        except Exception:
            await session.rollback()
            continue
    raise PairingError("could not allocate a free pairing code")


async def claim_atomic(
    session: AsyncSession,
    *,
    claim_code: str,
    device_id: str,
    mac: str,
    fw_version: str,
    profile_id: str,
    profile_sensor_specs: list[dict],
) -> ClaimResult:
    """Run the full claim sequence inside one transaction.

    1. SELECT FOR UPDATE the pairing_windows row, filtering on expiry + unused.
    2. Reject if device already has a credential.
    3. Upsert device, INSERT credential, UPDATE window, INSERT sensors (from profile).
    4. Return raw api_key (one-shot to client; only hash is persisted).
    """
    # Step 1: lock the window row atomically.
    locked = await session.execute(
        text(
            "SELECT code FROM pairing_windows "
            "WHERE code = :code AND expires_at > now() AND consumed_by IS NULL "
            "FOR UPDATE"
        ),
        {"code": claim_code},
    )
    if locked.first() is None:
        raise WindowNotAvailable("pairing window expired, used, or unknown")

    # Step 2: reject if device already has a live credential.
    existing_cred = (
        await session.execute(
            select(DeviceCredential.device_id).where(
                DeviceCredential.device_id == device_id
            )
        )
    ).scalar_one_or_none()
    if existing_cred is not None:
        raise DeviceAlreadyRegistered(device_id)

    # Step 3a: upsert device.
    dev_stmt = pg_insert(Device).values(
        device_id=device_id,
        fw_version=fw_version,
        profile_id=profile_id,
        first_seen_at=func.now(),
        last_seen_at=func.now(),
    )
    dev_stmt = dev_stmt.on_conflict_do_update(
        index_elements=[Device.device_id],
        set_={
            "fw_version": dev_stmt.excluded.fw_version,
            "profile_id": dev_stmt.excluded.profile_id,
            "last_seen_at": func.now(),
        },
    )
    await session.execute(dev_stmt)

    # Step 3b: insert device credential (api_key returned raw; hash persisted).
    api_key = secrets.token_hex(32)
    api_key_hash = hashlib.sha256(api_key.encode()).hexdigest()
    await session.execute(
        pg_insert(DeviceCredential).values(
            device_id=device_id,
            api_key_hash=api_key_hash,
            created_at=func.now(),
        )
    )

    # Step 3c: mark window consumed.
    await session.execute(
        text(
            "UPDATE pairing_windows "
            "SET consumed_by = :did, consumed_at = now() "
            "WHERE code = :code"
        ),
        {"did": device_id, "code": claim_code},
    )

    # Step 3d: pre-create sensors from profile.
    sensor_rows = [
        {
            "device_id": device_id,
            "channel_id": spec["channel_id"],
            "kind": spec["kind"],
            "unit": spec["unit"],
        }
        for spec in profile_sensor_specs
    ]
    if sensor_rows:
        await session.execute(
            pg_insert(Sensor).values(sensor_rows).on_conflict_do_nothing(
                index_elements=[Sensor.device_id, Sensor.channel_id, Sensor.kind]
            )
        )

    await session.commit()
    return ClaimResult(api_key=api_key, device_id=device_id)
