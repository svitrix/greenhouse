from fastapi import APIRouter

from app.routes import (
    admin_tokens,
    auth,
    commands,
    devices,
    events,
    health,
    ingest,
    locations,
    pairing,
    plant_groups,
    sensors,
)

api_router = APIRouter()
api_router.include_router(health.router)
api_router.include_router(ingest.router)
api_router.include_router(pairing.router)
api_router.include_router(locations.router)
api_router.include_router(plant_groups.router)
api_router.include_router(devices.router)
api_router.include_router(sensors.router)
api_router.include_router(admin_tokens.router)
api_router.include_router(auth.router)
api_router.include_router(commands.admin_router)
api_router.include_router(commands.device_router)
api_router.include_router(events.router)
