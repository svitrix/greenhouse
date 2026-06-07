"""locations, plant_groups, hierarchy columns on devices and sensors

Revision ID: 0003
Revises: 0002
Create Date: 2026-06-03 00:00:01.000000
"""
from alembic import op


revision = "0003"
down_revision = "0002"
branch_labels = None
depends_on = None


def upgrade() -> None:
    op.execute('CREATE EXTENSION IF NOT EXISTS "pgcrypto"')

    op.execute(
        """
        CREATE TABLE locations (
            id            UUID PRIMARY KEY DEFAULT gen_random_uuid(),
            name          TEXT NOT NULL,
            address       TEXT,
            timezone      TEXT NOT NULL DEFAULT 'UTC',
            notes         TEXT,
            created_at    TIMESTAMPTZ NOT NULL DEFAULT now()
        )
        """
    )

    op.execute(
        """
        CREATE TABLE plant_groups (
            id            UUID PRIMARY KEY DEFAULT gen_random_uuid(),
            location_id   UUID NOT NULL REFERENCES locations(id) ON DELETE CASCADE,
            name          TEXT NOT NULL,
            species       TEXT,
            planted_at    DATE,
            notes         TEXT,
            created_at    TIMESTAMPTZ NOT NULL DEFAULT now()
        )
        """
    )
    op.execute(
        "CREATE INDEX plant_groups_location_idx ON plant_groups(location_id)"
    )

    op.execute(
        """
        ALTER TABLE devices
          ADD COLUMN friendly_name TEXT,
          ADD COLUMN location_id   UUID REFERENCES locations(id) ON DELETE SET NULL
        """
    )
    op.execute("CREATE INDEX devices_location_idx ON devices(location_id)")

    op.execute(
        """
        ALTER TABLE sensors
          ADD COLUMN friendly_name      TEXT,
          ADD COLUMN plant_group_id     UUID REFERENCES plant_groups(id) ON DELETE SET NULL,
          ADD COLUMN calibration_json   JSONB,
          ADD COLUMN created_at         TIMESTAMPTZ NOT NULL DEFAULT now(),
          ADD COLUMN last_value         DOUBLE PRECISION,
          ADD COLUMN last_value_at      TIMESTAMPTZ
        """
    )
    op.execute(
        "CREATE INDEX sensors_plant_group_idx ON sensors(plant_group_id)"
    )


def downgrade() -> None:
    op.execute("DROP INDEX IF EXISTS sensors_plant_group_idx")
    op.execute(
        """
        ALTER TABLE sensors
          DROP COLUMN IF EXISTS last_value_at,
          DROP COLUMN IF EXISTS last_value,
          DROP COLUMN IF EXISTS created_at,
          DROP COLUMN IF EXISTS calibration_json,
          DROP COLUMN IF EXISTS plant_group_id,
          DROP COLUMN IF EXISTS friendly_name
        """
    )
    op.execute("DROP INDEX IF EXISTS devices_location_idx")
    op.execute(
        """
        ALTER TABLE devices
          DROP COLUMN IF EXISTS location_id,
          DROP COLUMN IF EXISTS friendly_name
        """
    )
    op.execute("DROP INDEX IF EXISTS plant_groups_location_idx")
    op.execute("DROP TABLE IF EXISTS plant_groups")
    op.execute("DROP TABLE IF EXISTS locations")
