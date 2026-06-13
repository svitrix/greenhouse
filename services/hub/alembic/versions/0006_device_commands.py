"""device_commands queue — reverse channel (D-5a)

The Telegram bot (separate `services/bot/` service) enqueues pump on/off
commands through the admin REST API into ``device_commands``; the
coordinator firmware will later poll ``status = 'pending'`` rows and ack
them. Subscriber state and the notification cursor live in the bot, not
here — the hub only owns the device-command queue.

Revision ID: 0006
Revises: 0005
Create Date: 2026-06-13 00:00:00.000000
"""
from alembic import op


revision = "0006"
down_revision = "0005"
branch_labels = None
depends_on = None


def upgrade() -> None:
    op.execute(
        """
        CREATE TABLE device_commands (
            id          TEXT PRIMARY KEY,
            device_id   TEXT NOT NULL
                        REFERENCES devices(device_id) ON DELETE CASCADE,
            command     TEXT NOT NULL,
            params_json JSONB,
            status      TEXT NOT NULL DEFAULT 'pending',
            created_by  TEXT,
            created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
            claimed_at  TIMESTAMPTZ,
            acked_at    TIMESTAMPTZ,
            result_json JSONB
        )
        """
    )
    # Partial index: the coordinator's hot path is "give me this device's
    # pending commands, oldest first".
    op.execute(
        """
        CREATE INDEX ix_device_commands_pending
            ON device_commands (device_id, created_at)
            WHERE status = 'pending'
        """
    )


def downgrade() -> None:
    op.execute("DROP TABLE IF EXISTS device_commands")
