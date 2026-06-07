from typing import Annotated, Literal

from pydantic import BaseModel, ConfigDict, Field


READING_KINDS = (
    "air_temp", "air_humidity", "soil_moist", "soil_temp", "battery_pct", "battery_v",
)
EVENT_KINDS = ("watered", "dry_run_aborted", "sensor_offline", "provisioned")

ReadingKind = Literal[
    "air_temp", "air_humidity", "soil_moist", "soil_temp", "battery_pct", "battery_v"
]
EventKind = Literal["watered", "dry_run_aborted", "sensor_offline", "provisioned"]


class ReadingIn(BaseModel):
    model_config = ConfigDict(extra="forbid")
    ts: Annotated[int, Field(ge=0, description="unix milliseconds UTC")]
    channel_id: Annotated[int, Field(ge=0, le=31)]
    kind: ReadingKind
    value: float
    raw: int | None = None
    status: Annotated[int, Field(ge=0, le=255)] = 0


class EventIn(BaseModel):
    model_config = ConfigDict(extra="forbid")
    ts: Annotated[int, Field(ge=0)]
    kind: EventKind
    payload: dict | None = None


class IngestBatch(BaseModel):
    model_config = ConfigDict(extra="forbid")
    device_id: Annotated[str, Field(min_length=3, max_length=64, pattern=r"^[a-z0-9-]+$")]
    fw_version: Annotated[str, Field(min_length=1, max_length=32)]
    batch_id: Annotated[str, Field(min_length=1, max_length=64)]
    readings: Annotated[list[ReadingIn], Field(min_length=1, max_length=2000)]
    events: list[EventIn] = []
