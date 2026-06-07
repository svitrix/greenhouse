import hashlib
from dataclasses import dataclass

from fastapi import Depends, Header, HTTPException, status
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from app.config import get_settings
from app.db import get_session
from app.models import DeviceCredential


@dataclass(frozen=True)
class AuthenticatedDevice:
    device_id: str


def _extract_bearer(authorization: str | None) -> str:
    if not authorization or not authorization.lower().startswith("bearer "):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED, detail="missing bearer"
        )
    return authorization.split(" ", 1)[1].strip()


async def _lookup_db(token: str, session: AsyncSession) -> str | None:
    digest = hashlib.sha256(token.encode("utf-8")).hexdigest()
    result = await session.execute(
        select(DeviceCredential.device_id).where(
            DeviceCredential.api_key_hash == digest
        )
    )
    return result.scalar_one_or_none()


async def verify_device(
    authorization: str | None = Header(default=None),
    session: AsyncSession = Depends(get_session),
) -> AuthenticatedDevice:
    token = _extract_bearer(authorization)

    # 1) env map (dev / local)
    env_map = get_settings().device_api_keys
    for device_id, key in env_map.items():
        if key == token:
            return AuthenticatedDevice(device_id=device_id)

    # 2) DB-backed credentials
    device_id = await _lookup_db(token, session)
    if device_id:
        return AuthenticatedDevice(device_id=device_id)

    raise HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED, detail="invalid bearer"
    )
