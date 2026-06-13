from datetime import datetime

from pydantic import BaseModel


class EventOut(BaseModel):
    ts:          datetime
    device_id:   str
    device_name: str | None
    kind:        str
    payload:     dict | None
