from datetime import date, datetime
from typing import Any, Optional
from uuid import UUID

from sqlalchemy import ForeignKey, SmallInteger, Text
from sqlalchemy.dialects.postgresql import JSONB, TIMESTAMP, UUID as PgUUID
from sqlalchemy.orm import DeclarativeBase, Mapped, mapped_column


class Base(DeclarativeBase):
    pass


class Device(Base):
    __tablename__ = "devices"

    device_id:     Mapped[str] = mapped_column(Text, primary_key=True)
    display_name:  Mapped[Optional[str]] = mapped_column(Text, nullable=True)
    fw_version:    Mapped[Optional[str]] = mapped_column(Text, nullable=True)
    first_seen_at: Mapped[datetime] = mapped_column(TIMESTAMP(timezone=True))
    last_seen_at:  Mapped[datetime] = mapped_column(TIMESTAMP(timezone=True))
    # Added by 0003_hierarchy migration:
    friendly_name: Mapped[Optional[str]] = mapped_column(Text, nullable=True)
    location_id:   Mapped[Optional[UUID]] = mapped_column(
        PgUUID(as_uuid=True),
        ForeignKey("locations.id", ondelete="SET NULL"),
        nullable=True,
    )
    # Added by 0004_device_profiles migration (NOT NULL after backfill):
    profile_id:    Mapped[str] = mapped_column(
        Text, ForeignKey("device_profiles.profile_id")
    )


class Sensor(Base):
    __tablename__ = "sensors"

    device_id:  Mapped[str] = mapped_column(
        Text, ForeignKey("devices.device_id", ondelete="CASCADE"), primary_key=True
    )
    channel_id: Mapped[int] = mapped_column(SmallInteger, primary_key=True)
    kind:       Mapped[str] = mapped_column(Text, primary_key=True)
    unit:       Mapped[str] = mapped_column(Text)
    # Added by 0003_hierarchy migration:
    friendly_name:    Mapped[Optional[str]] = mapped_column(Text, nullable=True)
    plant_group_id:   Mapped[Optional[UUID]] = mapped_column(
        PgUUID(as_uuid=True),
        ForeignKey("plant_groups.id", ondelete="SET NULL"),
        nullable=True,
    )
    calibration_json: Mapped[Optional[dict[str, Any]]] = mapped_column(JSONB, nullable=True)
    created_at:       Mapped[datetime] = mapped_column(TIMESTAMP(timezone=True))
    last_value:       Mapped[Optional[float]] = mapped_column(nullable=True)
    last_value_at:    Mapped[Optional[datetime]] = mapped_column(
        TIMESTAMP(timezone=True), nullable=True
    )


class Reading(Base):
    __tablename__ = "readings"

    device_id:  Mapped[str] = mapped_column(Text, primary_key=True)
    channel_id: Mapped[int] = mapped_column(SmallInteger, primary_key=True)
    kind:       Mapped[str] = mapped_column(Text, primary_key=True)
    ts:         Mapped[datetime] = mapped_column(TIMESTAMP(timezone=True), primary_key=True)
    value:      Mapped[float]
    raw:        Mapped[Optional[int]] = mapped_column(nullable=True)
    status:     Mapped[int] = mapped_column(SmallInteger, default=0)


class Event(Base):
    __tablename__ = "events"

    # Hypertable has no real PRIMARY KEY constraint at the SQL level.
    # The composite identity here exists only so SQLAlchemy can identify
    # rows in a session; INSERTs still append freely.
    ts:           Mapped[datetime] = mapped_column(TIMESTAMP(timezone=True), primary_key=True)
    device_id:    Mapped[str] = mapped_column(Text, primary_key=True)
    kind:         Mapped[str] = mapped_column(Text, primary_key=True)
    payload_json: Mapped[Optional[dict[str, Any]]] = mapped_column(JSONB, nullable=True)


class DeviceCredential(Base):
    __tablename__ = "device_credentials"

    device_id:    Mapped[str] = mapped_column(
        Text, ForeignKey("devices.device_id", ondelete="CASCADE"), primary_key=True
    )
    api_key_hash: Mapped[str] = mapped_column(Text)
    created_at:   Mapped[datetime] = mapped_column(TIMESTAMP(timezone=True))


class AdminToken(Base):
    __tablename__ = "admin_tokens"

    token_hash:   Mapped[str] = mapped_column(Text, primary_key=True)
    name:         Mapped[Optional[str]] = mapped_column(Text, nullable=True)
    created_at:   Mapped[datetime] = mapped_column(TIMESTAMP(timezone=True))
    last_used_at: Mapped[Optional[datetime]] = mapped_column(
        TIMESTAMP(timezone=True), nullable=True
    )


class PairingWindow(Base):
    __tablename__ = "pairing_windows"

    code:         Mapped[str] = mapped_column(Text, primary_key=True)
    opens_at:     Mapped[datetime] = mapped_column(TIMESTAMP(timezone=True))
    expires_at:   Mapped[datetime] = mapped_column(TIMESTAMP(timezone=True))
    consumed_by:  Mapped[Optional[str]] = mapped_column(Text, nullable=True)
    consumed_at:  Mapped[Optional[datetime]] = mapped_column(
        TIMESTAMP(timezone=True), nullable=True
    )


class Location(Base):
    __tablename__ = "locations"

    id:         Mapped[UUID] = mapped_column(PgUUID(as_uuid=True), primary_key=True)
    name:       Mapped[str] = mapped_column(Text)
    address:    Mapped[Optional[str]] = mapped_column(Text, nullable=True)
    timezone:   Mapped[str] = mapped_column(Text)
    notes:      Mapped[Optional[str]] = mapped_column(Text, nullable=True)
    created_at: Mapped[datetime] = mapped_column(TIMESTAMP(timezone=True))


class PlantGroup(Base):
    __tablename__ = "plant_groups"

    id:          Mapped[UUID] = mapped_column(PgUUID(as_uuid=True), primary_key=True)
    location_id: Mapped[UUID] = mapped_column(
        PgUUID(as_uuid=True),
        ForeignKey("locations.id", ondelete="CASCADE"),
    )
    name:        Mapped[str] = mapped_column(Text)
    species:     Mapped[Optional[str]] = mapped_column(Text, nullable=True)
    planted_at:  Mapped[Optional[date]] = mapped_column(nullable=True)
    notes:       Mapped[Optional[str]] = mapped_column(Text, nullable=True)
    created_at:  Mapped[datetime] = mapped_column(TIMESTAMP(timezone=True))


class DeviceProfile(Base):
    __tablename__ = "device_profiles"

    profile_id:   Mapped[str] = mapped_column(Text, primary_key=True)
    name:         Mapped[str] = mapped_column(Text)
    description:  Mapped[Optional[str]] = mapped_column(Text, nullable=True)
    manufacturer: Mapped[str] = mapped_column(Text)
    sensor_specs: Mapped[list[dict[str, Any]]] = mapped_column(JSONB)
    created_at:   Mapped[datetime] = mapped_column(TIMESTAMP(timezone=True))


class AdminUser(Base):
    __tablename__ = "admin_users"

    username:      Mapped[str] = mapped_column(Text, primary_key=True)
    password_hash: Mapped[str] = mapped_column(Text)
    created_at:    Mapped[datetime] = mapped_column(TIMESTAMP(timezone=True))
