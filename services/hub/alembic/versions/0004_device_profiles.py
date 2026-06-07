"""device_profiles catalog + ALTER devices ADD profile_id

Revision ID: 0004
Revises: 0003
Create Date: 2026-06-03 00:00:02.000000
"""
from alembic import op


revision = "0004"
down_revision = "0003"
branch_labels = None
depends_on = None


_COORDINATOR_V1_SPECS = """
[
    {"channel_id": 0, "kind": "air_temp",     "unit": "°C", "range_min": -40, "range_max": 85,  "description": "AM2315C air temperature"},
    {"channel_id": 0, "kind": "air_humidity", "unit": "%",  "range_min": 0,   "range_max": 100, "description": "AM2315C relative humidity"},
    {"channel_id": 1, "kind": "soil_moist",   "unit": "%",  "range_min": 0,   "range_max": 100, "description": "Chirp soil moisture (normalised)"},
    {"channel_id": 1, "kind": "soil_temp",    "unit": "°C", "range_min": -20, "range_max": 60,  "description": "Chirp soil temperature"},
    {"channel_id": 2, "kind": "battery_pct",  "unit": "%",  "range_min": 0,   "range_max": 100, "description": "LiPo SoC estimate"},
    {"channel_id": 2, "kind": "battery_v",    "unit": "V",  "range_min": 3.0, "range_max": 4.3, "description": "LiPo voltage"}
]
"""


def upgrade() -> None:
    op.execute(
        """
        CREATE TABLE device_profiles (
            profile_id   TEXT PRIMARY KEY,
            name         TEXT NOT NULL,
            description  TEXT,
            manufacturer TEXT NOT NULL DEFAULT 'svitrix',
            sensor_specs JSONB NOT NULL,
            created_at   TIMESTAMPTZ NOT NULL DEFAULT now()
        )
        """
    )

    op.execute(
        f"""
        INSERT INTO device_profiles (profile_id, name, description, sensor_specs)
        VALUES (
            'gh-coordinator-v1',
            'Greenhouse Coordinator v1',
            'ESP32-C6 coordinator forwarding telemetry from one AM2315C + Chirp + battery sensor-node',
            $${_COORDINATOR_V1_SPECS.strip()}$$::jsonb
        )
        """
    )

    # Add profile_id to devices, nullable first so the backfill works.
    op.execute(
        "ALTER TABLE devices ADD COLUMN profile_id TEXT "
        "REFERENCES device_profiles(profile_id)"
    )
    # Backfill existing devices with the v1 profile (everything pre-D-2).
    op.execute(
        "UPDATE devices SET profile_id = 'gh-coordinator-v1' WHERE profile_id IS NULL"
    )
    # Now apply the NOT NULL constraint.
    op.execute("ALTER TABLE devices ALTER COLUMN profile_id SET NOT NULL")
    op.execute("CREATE INDEX devices_profile_idx ON devices(profile_id)")


def downgrade() -> None:
    op.execute("DROP INDEX IF EXISTS devices_profile_idx")
    op.execute("ALTER TABLE devices DROP COLUMN IF EXISTS profile_id")
    op.execute("DROP TABLE IF EXISTS device_profiles")
