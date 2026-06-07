import pytest
from pydantic import ValidationError

from app.schemas.ingest import EventIn, IngestBatch, ReadingIn


def _good_reading(**overrides):
    base = {
        "ts": 1748774103000,
        "channel_id": 0,
        "kind": "air_temp",
        "value": 23.4,
        "status": 0,
    }
    return {**base, **overrides}


def test_batch_accepts_valid_payload():
    payload = {
        "device_id": "gh-a1b2c3d4",
        "fw_version": "1.2.3",
        "batch_id": "01HF9K3M4N5P6Q7R8S9T0V",
        "readings": [_good_reading()],
        "events": [],
    }
    batch = IngestBatch.model_validate(payload)
    assert batch.device_id == "gh-a1b2c3d4"
    assert len(batch.readings) == 1


def test_reading_rejects_unknown_kind():
    with pytest.raises(ValidationError, match="kind"):
        ReadingIn.model_validate(_good_reading(kind="unknown_kind"))


def test_reading_rejects_negative_channel_id():
    with pytest.raises(ValidationError, match="channel_id"):
        ReadingIn.model_validate(_good_reading(channel_id=-1))


def test_event_kind_is_allowlisted():
    EventIn.model_validate({"ts": 1, "kind": "watered", "payload": {"duration_ms": 8000}})
    with pytest.raises(ValidationError):
        EventIn.model_validate({"ts": 1, "kind": "unknown_event"})


def test_empty_batch_is_invalid():
    with pytest.raises(ValidationError):
        IngestBatch.model_validate({
            "device_id": "gh-x",
            "fw_version": "1.0.0",
            "batch_id": "01H",
            "readings": [],
            "events": [],
        })
