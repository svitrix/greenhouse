from datetime import datetime
from typing import Annotated

from pydantic import BaseModel, ConfigDict, Field


class PairingOpenIn(BaseModel):
    model_config = ConfigDict(extra="forbid")
    ttl_seconds: Annotated[int, Field(ge=60, le=3600, default=300)]


class PairingOpenOut(BaseModel):
    code: str
    expires_at: datetime


class PairingClaimIn(BaseModel):
    model_config = ConfigDict(extra="forbid")
    claim_code: Annotated[str, Field(pattern=r"^\d{6}$")]
    device_id:  Annotated[str, Field(min_length=3, max_length=64, pattern=r"^[a-z0-9-]+$")]
    mac:        Annotated[str, Field(pattern=r"^[0-9a-f]{2}(:[0-9a-f]{2}){5}$")]
    fw_version: Annotated[str, Field(min_length=1, max_length=32)]
    profile_id: Annotated[str, Field(min_length=3, max_length=64)]


class PairingClaimOut(BaseModel):
    api_key: str
    device_id: str


class PairingWindowOut(BaseModel):
    code: str
    opens_at: datetime
    expires_at: datetime
    consumed_by: str | None
