from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.ext.asyncio import AsyncSession

from app.auth import AuthenticatedDevice, verify_device
from app.auth_admin import AuthenticatedAdmin, verify_admin
from app.db import get_session
from app.repositories import commands as repo
from app.schemas.commands import CommandAck, CommandCreate, CommandOut, PendingCommand


# Admin-facing surface lives under /api/devices/{id}/commands; the device
# poll/ack surface is a sibling so it can carry device-bearer auth without
# colliding with the admin dependency on the parent path.
admin_router = APIRouter(prefix="/api/devices", tags=["commands"])
device_router = APIRouter(prefix="/api", tags=["commands"])


@admin_router.post(
    "/{device_id}/commands",
    response_model=CommandOut,
    status_code=status.HTTP_201_CREATED,
)
async def enqueue_command(
    device_id: str,
    body: CommandCreate,
    admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> CommandOut:
    try:
        row = await repo.enqueue(
            session,
            device_id=device_id,
            command=body.command,
            params=body.params,
            created_by=admin.name,
        )
    except repo.DeviceNotFound:
        raise HTTPException(status_code=404, detail="device not found")
    return CommandOut.model_validate(row)


@admin_router.get("/{device_id}/commands", response_model=list[CommandOut])
async def list_commands(
    device_id: str,
    _admin: AuthenticatedAdmin = Depends(verify_admin),
    session: AsyncSession = Depends(get_session),
) -> list[CommandOut]:
    rows = await repo.list_for_device(session, device_id)
    return [CommandOut.model_validate(r) for r in rows]


@device_router.get(
    "/devices/{device_id}/commands/pending", response_model=list[PendingCommand]
)
async def poll_pending(
    device_id: str,
    device: AuthenticatedDevice = Depends(verify_device),
    session: AsyncSession = Depends(get_session),
) -> list[PendingCommand]:
    if device_id != device.device_id:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="device_id does not match credential owner",
        )
    rows = await repo.claim_pending(session, device_id)
    return [PendingCommand.model_validate(r) for r in rows]


@device_router.post("/commands/{command_id}/ack", response_model=CommandOut)
async def ack_command(
    command_id: str,
    body: CommandAck,
    device: AuthenticatedDevice = Depends(verify_device),
    session: AsyncSession = Depends(get_session),
) -> CommandOut:
    row = await repo.ack(
        session,
        command_id,
        device_id=device.device_id,
        status=body.status,
        result=body.result,
    )
    if row is None:
        raise HTTPException(status_code=404, detail="command not found for this device")
    return CommandOut.model_validate(row)
