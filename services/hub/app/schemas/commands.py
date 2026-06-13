from datetime import datetime
from typing import Literal

from pydantic import BaseModel, ConfigDict, Field


# Reverse-channel commands the hub can hand to a coordinator. Kept a closed
# Literal so the bot cannot enqueue something the firmware will never honour.
CommandKind = Literal["pump_on", "pump_off"]
CommandStatus = Literal["pending", "sent", "acked", "failed", "expired"]


class CommandCreate(BaseModel):
    model_config = ConfigDict(extra="forbid")
    command:     CommandKind
    # Optional knobs, e.g. {"duration_ms": 5000} for pump_on. The firmware
    # clamps to its own max runtime regardless of what is requested here.
    params:      dict | None = None


class CommandAck(BaseModel):
    model_config = ConfigDict(extra="forbid")
    status: Literal["acked", "failed"]
    result: dict | None = None


class CommandOut(BaseModel):
    model_config = ConfigDict(from_attributes=True)
    id:          str
    device_id:   str
    command:     str
    params_json: dict | None
    status:      str
    created_by:  str | None
    created_at:  datetime
    claimed_at:  datetime | None
    acked_at:    datetime | None
    result_json: dict | None


class PendingCommand(BaseModel):
    """Slim shape handed to the coordinator on its poll — only what the
    firmware needs to act and ack. `params` reads the ORM's `params_json`
    column (validation alias) but serialises back out as `params`."""
    model_config = ConfigDict(from_attributes=True)
    id:      str
    command: str
    params:  dict | None = Field(default=None, validation_alias="params_json")
