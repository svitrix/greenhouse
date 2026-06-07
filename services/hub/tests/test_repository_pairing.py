import asyncio
from datetime import datetime, timezone

import pytest
from sqlalchemy import text
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.repositories.pairing import (
    DeviceAlreadyRegistered,
    WindowNotAvailable,
    claim_atomic,
    open_window,
)


_PROFILE_SPECS = [
    {"channel_id": 0, "kind": "air_temp",     "unit": "°C"},
    {"channel_id": 0, "kind": "air_humidity", "unit": "%"},
    {"channel_id": 1, "kind": "soil_moist",   "unit": "%"},
]


@pytest.mark.asyncio
async def test_open_window_returns_6_digit_code(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        w = await open_window(s, ttl_seconds=300)
    assert len(w.code) == 6
    assert w.code.isdigit()
    assert w.expires_at > datetime.now(tz=timezone.utc)


@pytest.mark.asyncio
async def test_claim_happy_path_creates_device_credential_and_sensors(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        w = await open_window(s, ttl_seconds=300)
    async with maker() as s:
        res = await claim_atomic(
            s,
            claim_code=w.code,
            device_id="gh-test01",
            mac="aa:bb:cc:dd:ee:ff",
            fw_version="0.4.0",
            profile_id="gh-coordinator-v1",
            profile_sensor_specs=_PROFILE_SPECS,
        )
    assert len(res.api_key) == 64
    assert res.device_id == "gh-test01"

    async with maker() as s:
        cred = (
            await s.execute(
                text("SELECT device_id FROM device_credentials WHERE device_id='gh-test01'")
            )
        ).scalar_one()
        assert cred == "gh-test01"
        sensors = (
            await s.execute(
                text("SELECT count(*) FROM sensors WHERE device_id='gh-test01'")
            )
        ).scalar_one()
        assert sensors == 3


@pytest.mark.asyncio
async def test_claim_expired_window_is_rejected(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        # insert an already-expired window manually
        await s.execute(
            text(
                "INSERT INTO pairing_windows (code, opens_at, expires_at) "
                "VALUES ('111111', now() - interval '10 min', now() - interval '5 min')"
            )
        )
        await s.commit()

    async with maker() as s:
        with pytest.raises(WindowNotAvailable):
            await claim_atomic(
                s,
                claim_code="111111",
                device_id="gh-late",
                mac="aa:bb:cc:dd:ee:ff",
                fw_version="0.4.0",
                profile_id="gh-coordinator-v1",
                profile_sensor_specs=_PROFILE_SPECS,
            )


@pytest.mark.asyncio
async def test_claim_already_registered_is_rejected(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        w1 = await open_window(s, ttl_seconds=300)
    async with maker() as s:
        await claim_atomic(
            s, claim_code=w1.code, device_id="gh-dup", mac="aa:bb:cc:dd:ee:ff",
            fw_version="0.4.0", profile_id="gh-coordinator-v1",
            profile_sensor_specs=_PROFILE_SPECS,
        )
    async with maker() as s:
        w2 = await open_window(s, ttl_seconds=300)
    async with maker() as s:
        with pytest.raises(DeviceAlreadyRegistered):
            await claim_atomic(
                s, claim_code=w2.code, device_id="gh-dup", mac="aa:bb:cc:dd:ee:ff",
                fw_version="0.5.0", profile_id="gh-coordinator-v1",
                profile_sensor_specs=_PROFILE_SPECS,
            )


@pytest.mark.asyncio
async def test_race_two_parallel_claims_exactly_one_wins(migrated_db):
    maker = async_sessionmaker(migrated_db, expire_on_commit=False)
    async with maker() as s:
        w = await open_window(s, ttl_seconds=300)

    async def attempt(device_id: str):
        async with maker() as s2:
            try:
                return await claim_atomic(
                    s2, claim_code=w.code, device_id=device_id,
                    mac="aa:bb:cc:dd:ee:ff", fw_version="0.4.0",
                    profile_id="gh-coordinator-v1",
                    profile_sensor_specs=_PROFILE_SPECS,
                )
            except WindowNotAvailable:
                return None

    a, b = await asyncio.gather(attempt("gh-A"), attempt("gh-B"))
    winners = [x for x in (a, b) if x is not None]
    assert len(winners) == 1, f"expected exactly one winner, got {winners}"
