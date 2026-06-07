"""initial schema

Revision ID: 0001
Revises:
Create Date: 2026-06-01 00:00:00.000000
"""
from alembic import op


revision = "0001"
down_revision = None
branch_labels = None
depends_on = None


def upgrade() -> None:
    op.execute("CREATE EXTENSION IF NOT EXISTS timescaledb")

    op.execute(
        """
        CREATE TABLE devices (
            device_id     TEXT PRIMARY KEY,
            display_name  TEXT,
            fw_version    TEXT,
            first_seen_at TIMESTAMPTZ NOT NULL DEFAULT now(),
            last_seen_at  TIMESTAMPTZ NOT NULL DEFAULT now()
        )
        """
    )

    op.execute(
        """
        CREATE TABLE sensors (
            device_id   TEXT     NOT NULL REFERENCES devices(device_id) ON DELETE CASCADE,
            channel_id  SMALLINT NOT NULL,
            kind        TEXT     NOT NULL,
            unit        TEXT     NOT NULL,
            PRIMARY KEY (device_id, channel_id, kind)
        )
        """
    )

    op.execute(
        """
        CREATE TABLE readings (
            ts          TIMESTAMPTZ      NOT NULL,
            device_id   TEXT             NOT NULL,
            channel_id  SMALLINT         NOT NULL,
            kind        TEXT             NOT NULL,
            value       DOUBLE PRECISION NOT NULL,
            raw         INTEGER,
            status      SMALLINT         NOT NULL DEFAULT 0,
            PRIMARY KEY (device_id, channel_id, kind, ts)
        )
        """
    )
    op.execute("SELECT create_hypertable('readings', 'ts')")
    op.execute(
        "CREATE INDEX readings_device_kind_ts_desc ON readings (device_id, kind, ts DESC)"
    )

    op.execute(
        """
        CREATE TABLE events (
            ts           TIMESTAMPTZ NOT NULL DEFAULT now(),
            device_id    TEXT NOT NULL,
            kind         TEXT NOT NULL,
            payload_json JSONB
        )
        """
    )
    op.execute("SELECT create_hypertable('events', 'ts')")
    op.execute("CREATE INDEX events_device_ts_desc ON events (device_id, ts DESC)")

    op.execute(
        """
        CREATE TABLE device_credentials (
            device_id    TEXT PRIMARY KEY REFERENCES devices(device_id) ON DELETE CASCADE,
            api_key_hash TEXT NOT NULL,
            created_at   TIMESTAMPTZ NOT NULL DEFAULT now()
        )
        """
    )


def downgrade() -> None:
    op.execute("DROP TABLE IF EXISTS device_credentials")
    op.execute("DROP TABLE IF EXISTS events")
    op.execute("DROP TABLE IF EXISTS readings")
    op.execute("DROP TABLE IF EXISTS sensors")
    op.execute("DROP TABLE IF EXISTS devices")
