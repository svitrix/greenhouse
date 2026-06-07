from datetime import datetime
from typing import Annotated
from uuid import UUID

from pydantic import BaseModel, ConfigDict, Field


class LocationIn(BaseModel):
    model_config = ConfigDict(extra="forbid")
    name:     Annotated[str, Field(min_length=1, max_length=128)]
    address:  str | None = None
    timezone: Annotated[str, Field(default="UTC", max_length=64)]
    notes:    str | None = None


class LocationPatch(BaseModel):
    model_config = ConfigDict(extra="forbid")
    name:     Annotated[str, Field(min_length=1, max_length=128)] | None = None
    address:  str | None = None
    timezone: str | None = None
    notes:    str | None = None


class LocationOut(LocationIn):
    id: UUID
    created_at: datetime
