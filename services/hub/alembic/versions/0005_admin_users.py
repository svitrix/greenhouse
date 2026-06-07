"""admin_users table for username/password login (D-3a)

Revision ID: 0005
Revises: 0004
Create Date: 2026-06-03 00:00:03.000000
"""
from alembic import op


revision = "0005"
down_revision = "0004"
branch_labels = None
depends_on = None


def upgrade() -> None:
    op.execute(
        """
        CREATE TABLE admin_users (
            username      TEXT PRIMARY KEY,
            password_hash TEXT NOT NULL,
            created_at    TIMESTAMPTZ NOT NULL DEFAULT now()
        )
        """
    )


def downgrade() -> None:
    op.execute("DROP TABLE IF EXISTS admin_users")
