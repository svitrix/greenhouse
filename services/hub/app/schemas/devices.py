from datetime import datetime
from uuid import UUID

from pydantic import BaseModel, ConfigDict


class DevicePatch(BaseModel):
    model_config = ConfigDict(extra="forbid")
    friendly_name: str | None = None
    location_id:   UUID | None = None


class DeviceOut(BaseModel):
    device_id:     str
    friendly_name: str | None
    fw_version:    str | None
    profile_id:    str
    location_id:   UUID | None
    last_seen_at:  datetime
    sensors_count: int
    online:        bool
