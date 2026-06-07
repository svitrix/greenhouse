"""admin tokens + pairing windows

Revision ID: 0002
Revises: 0001
Create Date: 2026-06-03 00:00:00.000000
"""
from alembic import op


revision = "0002"
down_revision = "0001"
branch_labels = None
depends_on = None


def upgrade() -> None:
    op.execute(
        """
        CREATE TABLE admin_tokens (
            token_hash    TEXT PRIMARY KEY,
            name          TEXT,
            created_at    TIMESTAMPTZ NOT NULL DEFAULT now(),
            last_used_at  TIMESTAMPTZ
        )
        """
    )
    op.execute(
        "CREATE INDEX admin_tokens_last_used_idx ON admin_tokens(last_used_at)"
    )

    op.execute(
        """
        CREATE TABLE pairing_windows (
            code          TEXT PRIMARY KEY,
            opens_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
            expires_at    TIMESTAMPTZ NOT NULL,
            consumed_by   TEXT,
            consumed_at   TIMESTAMPTZ
        )
        """
    )
    op.execute(
        "CREATE INDEX pairing_windows_expires_idx "
        "ON pairing_windows(expires_at) WHERE consumed_by IS NULL"
    )


def downgrade() -> None:
    op.execute("DROP TABLE IF EXISTS pairing_windows")
    op.execute("DROP TABLE IF EXISTS admin_tokens")
