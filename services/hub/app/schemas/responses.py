from pydantic import BaseModel


class IngestResponse(BaseModel):
    accepted_readings: int
    accepted_events: int
    duplicates_skipped: int


class HealthResponse(BaseModel):
    status: str
    db: str
