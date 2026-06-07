from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from app.auth_admin import AuthenticatedAdmin, verify_admin
from app.db import get_session
from app.models import PairingWindow
from app.repositories.device_profiles import get_specs
from app.repositories.pairing import (
    DeviceAlreadyRegistered,
    PairingError,
    WindowNotAvailable,
    claim_atomic,
    open_window,
)
from app.schemas.pairing import (
    PairingClaimIn,
    PairingClaimOut,
    PairingOpenIn,
    PairingOpenOut,
    PairingWindowOut,
)


router = APIRouter(prefix="/api/pairing", tags=["pairing"])


@router.post("/open", response_model=PairingOpenOut)
async def open_pairing(
    body: PairingOpenIn,
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> PairingOpenOut:
    w = await open_window(session, ttl_seconds=body.ttl_seconds)
    return PairingOpenOut(code=w.code, expires_at=w.expires_at)


@router.post("/claim", response_model=PairingClaimOut)
async def claim_pairing(
    body: PairingClaimIn,
    session: AsyncSession = Depends(get_session),
) -> PairingClaimOut:
    specs = await get_specs(session, body.profile_id)
    if specs is None:
        raise HTTPException(
            status_code=422,
            detail=(
                f"profile {body.profile_id} not registered with this hub; "
                "firmware too new or unsupported"
            ),
        )

    try:
        res = await claim_atomic(
            session,
            claim_code=body.claim_code,
            device_id=body.device_id,
            mac=body.mac,
            fw_version=body.fw_version,
            profile_id=body.profile_id,
            profile_sensor_specs=specs,
        )
    except WindowNotAvailable as e:
        raise HTTPException(status_code=410, detail=str(e))
    except DeviceAlreadyRegistered as e:
        raise HTTPException(
            status_code=409,
            detail=f"device {e} already registered; admin must revoke first",
        )
    except PairingError as e:
        raise HTTPException(status_code=e.http_status, detail=str(e))

    return PairingClaimOut(api_key=res.api_key, device_id=res.device_id)


@router.get("/windows", response_model=list[PairingWindowOut])
async def list_windows(
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> list[PairingWindowOut]:
    rows = (
        await session.execute(
            select(PairingWindow).order_by(PairingWindow.opens_at.desc()).limit(50)
        )
    ).scalars().all()
    return [
        PairingWindowOut(
            code=r.code,
            opens_at=r.opens_at,
            expires_at=r.expires_at,
            consumed_by=r.consumed_by,
        )
        for r in rows
    ]
