from datetime import datetime
from uuid import UUID

from pydantic import BaseModel, ConfigDict


class SensorPatch(BaseModel):
    model_config = ConfigDict(extra="forbid")
    friendly_name:    str | None = None
    plant_group_id:   UUID | None = None
    calibration_json: dict | None = None


class SensorOut(BaseModel):
    device_id:        str
    channel_id:       int
    kind:             str
    unit:             str
    friendly_name:    str | None
    plant_group_id:   UUID | None
    calibration_json: dict | None
    last_value:       float | None
    last_value_at:    datetime | None
    created_at:       datetime
