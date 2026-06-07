from datetime import date, datetime
from typing import Annotated
from uuid import UUID

from pydantic import BaseModel, ConfigDict, Field


class PlantGroupIn(BaseModel):
    model_config = ConfigDict(extra="forbid")
    location_id: UUID
    name:        Annotated[str, Field(min_length=1, max_length=128)]
    species:     str | None = None
    planted_at:  date | None = None
    notes:       str | None = None


class PlantGroupPatch(BaseModel):
    model_config = ConfigDict(extra="forbid")
    name:       Annotated[str, Field(min_length=1, max_length=128)] | None = None
    species:    str | None = None
    planted_at: date | None = None
    notes:      str | None = None


class PlantGroupOut(BaseModel):
    id:          UUID
    location_id: UUID
    name:        str
    species:     str | None
    planted_at:  date | None
    notes:       str | None
    created_at:  datetime
