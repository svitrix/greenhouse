import time

from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.ext.asyncio import AsyncSession

from app.auth import AuthenticatedDevice, verify_device
from app.config import get_settings
from app.db import get_session
from app.repositories.ingest import UnknownSensorForDevice, insert_batch
from app.schemas.ingest import IngestBatch
from app.schemas.responses import IngestResponse


router = APIRouter(tags=["ingest"])


@router.post("/ingest", response_model=IngestResponse)
async def ingest(
    batch: IngestBatch,
    device: AuthenticatedDevice = Depends(verify_device),
    session: AsyncSession = Depends(get_session),
) -> IngestResponse:
    if batch.device_id != device.device_id:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="device_id in body does not match credential owner",
        )

    settings = get_settings()
    now_ms = int(time.time() * 1000)
    tol_ms = settings.TS_TOLERANCE_HOURS * 3600 * 1000
    for r in batch.readings:
        if r.ts < now_ms - tol_ms or r.ts > now_ms + tol_ms:
            raise HTTPException(
                status_code=status.HTTP_400_BAD_REQUEST,
                detail=(
                    f"reading ts {r.ts} outside "
                    f"±{settings.TS_TOLERANCE_HOURS}h window"
                ),
            )

    try:
        result = await insert_batch(session, batch)
    except UnknownSensorForDevice as e:
        raise HTTPException(
            status_code=400,
            detail=(
                f"unknown_sensor_for_device: {e.triplets}; "
                "device must be paired with a profile that declares these sensors"
            ),
        )
    await session.commit()
    return IngestResponse(
        accepted_readings=result.accepted_readings,
        accepted_events=result.accepted_events,
        duplicates_skipped=result.duplicates_skipped,
    )
