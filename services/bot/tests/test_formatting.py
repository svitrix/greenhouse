from datetime import datetime, timedelta, timezone

from bot.ui.formatting import (
    format_device_button,
    format_device_card,
    format_event,
    format_sensor_line,
    humanize_age,
)

NOW = datetime(2026, 6, 13, 12, 0, 0, tzinfo=timezone.utc)


def _iso(delta: timedelta) -> str:
    return (NOW - delta).isoformat()


def test_humanize_age_buckets_en():
    assert humanize_age(None, lang="en") == "no data"
    assert humanize_age(_iso(timedelta(seconds=10)), lang="en", now=NOW) == "10s ago"
    assert humanize_age(_iso(timedelta(minutes=5)), lang="en", now=NOW) == "5m ago"
    assert humanize_age(_iso(timedelta(hours=3)), lang="en", now=NOW) == "3h ago"
    assert humanize_age(_iso(timedelta(days=2)), lang="en", now=NOW) == "2d ago"


def test_humanize_age_buckets_uk():
    assert humanize_age(None, lang="uk") == "немає даних"
    assert humanize_age(_iso(timedelta(minutes=5)), lang="uk", now=NOW) == "5 хв тому"


def test_humanize_age_future_clamps_to_zero():
    assert humanize_age(_iso(timedelta(seconds=-30)), lang="en", now=NOW) == "0s ago"


def test_format_sensor_line_localised_label():
    sensor = {
        "friendly_name": None, "kind": "air_temp", "unit": "°C",
        "last_value": 21.5, "last_value_at": _iso(timedelta(minutes=2)),
    }
    en = format_sensor_line(sensor, lang="en", now=NOW)
    assert "21.5 °C" in en
    assert "2m ago" in en
    assert "Air temp." in en
    uk = format_sensor_line(sensor, lang="uk", now=NOW)
    assert "Темп. повітря" in uk
    assert "2 хв тому" in uk


def test_format_sensor_line_friendly_name_overrides():
    sensor = {
        "friendly_name": "Грядка", "kind": "soil_moist", "unit": "%",
        "last_value": None, "last_value_at": None,
    }
    assert format_sensor_line(sensor, lang="en", now=NOW) == "• Грядка: —"


def test_format_sensor_line_escapes_html():
    sensor = {
        "friendly_name": "<b>x</b>", "kind": "soil_moist", "unit": "%",
        "last_value": 40, "last_value_at": _iso(timedelta(seconds=5)),
    }
    line = format_sensor_line(sensor, lang="en", now=NOW)
    assert "<b>x</b>" not in line
    assert "&lt;b&gt;x&lt;/b&gt;" in line


def test_format_sensor_line_shows_band_state():
    dry = {
        "friendly_name": None, "kind": "soil_moist", "unit": "%",
        "last_value": 18, "last_value_at": _iso(timedelta(minutes=1)),
    }
    en = format_sensor_line(dry, lang="en", now=NOW)
    assert "⚠️" in en
    assert "dry · water me" in en
    uk = format_sensor_line(dry, lang="uk", now=NOW)
    assert "сухо · полийте" in uk


def test_format_sensor_line_no_band_for_battery():
    battery = {
        "friendly_name": None, "kind": "battery_pct", "unit": "%",
        "last_value": 87, "last_value_at": _iso(timedelta(minutes=1)),
    }
    line = format_sensor_line(battery, lang="en", now=NOW)
    assert "·" not in line  # no band segment, only "label: value (age)"


def test_format_device_card_empty_sensors():
    device = {
        "device_id": "gh-1", "friendly_name": "Greenhouse", "online": True,
        "last_seen_at": _iso(timedelta(minutes=2)),
    }
    en = format_device_card(device, [], lang="en", now=NOW)
    assert "🪴 <b>Greenhouse</b>" in en
    assert "🟢 online · 2m ago" in en
    assert "No registered sensors" in en
    uk = format_device_card(device, [], lang="uk", now=NOW)
    assert "Немає зареєстрованих датчиків" in uk


def test_format_device_card_renders_sensors_and_note():
    device = {
        "device_id": "gh-1", "friendly_name": "Tomato", "online": False,
        "last_seen_at": _iso(timedelta(hours=1)),
    }
    sensors = [
        {"friendly_name": None, "kind": "soil_moist", "unit": "%",
         "last_value": 55, "last_value_at": _iso(timedelta(minutes=3))},
    ]
    card = format_device_card(device, sensors, lang="en", now=NOW, note="✅ queued")
    assert "⚪️ offline" in card
    assert "✅ queued" in card
    assert "moist" in card  # 55% soil → moist band


def test_format_device_card_escapes_label():
    device = {"device_id": "gh-1", "friendly_name": "<b>x</b>", "online": True,
              "last_seen_at": _iso(timedelta(minutes=1))}
    card = format_device_card(device, [], lang="en", now=NOW)
    assert "<b>x</b>" not in card
    assert "&lt;b&gt;x&lt;/b&gt;" in card


def test_format_event_watered_with_duration():
    event = {
        "kind": "watered", "device_id": "gh-1", "device_name": "Greenhouse",
        "ts": _iso(timedelta(seconds=30)), "payload": {"duration_ms": 5000},
    }
    en = format_event(event, lang="en", now=NOW)
    assert "Watering started" in en
    assert "<b>Greenhouse</b>" in en
    assert "5s" in en
    uk = format_event(event, lang="uk", now=NOW)
    assert "Полив увімкнено" in uk
    assert "5 с" in uk


def test_format_device_button_online_offline():
    on = format_device_button({"device_id": "gh-1", "friendly_name": "A", "online": True})
    off = format_device_button({"device_id": "gh-1", "friendly_name": None, "online": False})
    assert on.startswith("🟢")
    assert off.startswith("⚪️")
    assert off.endswith("gh-1")
