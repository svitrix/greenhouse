"""Pure message-rendering helpers operating on the hub's JSON dicts.

Output is HTML (ParseMode.HTML) and every dynamic value is escaped, so a
device named "tomato_#3 <b>" can't break the markup or inject tags. Labels
and ages are localised via i18n; no telegram import here, so these stay
trivially unit-testable."""
from datetime import datetime, timezone
from html import escape

from bot import i18n
from bot.ui import sensor_state


def _parse_ts(value) -> datetime | None:
    if value is None:
        return None
    if isinstance(value, datetime):
        return value
    return datetime.fromisoformat(value)


def _label(lang: str, prefix: str, name: str) -> str:
    """Look up a localised label (kind_*, event_*); fall back to the raw
    name when there's no translation for it."""
    key = f"{prefix}_{name}"
    value = i18n.t(lang, key)
    return name if value == key else value


def humanize_age(value, *, lang: str = i18n.DEFAULT, now: datetime | None = None) -> str:
    then = _parse_ts(value)
    if then is None:
        return i18n.t(lang, "no_data")
    now = now or datetime.now(tz=timezone.utc)
    if then.tzinfo is None:
        then = then.replace(tzinfo=timezone.utc)
    seconds = max(0, int((now - then).total_seconds()))
    if seconds < 60:
        return i18n.t(lang, "age_sec", n=seconds)
    if seconds < 3600:
        return i18n.t(lang, "age_min", n=seconds // 60)
    if seconds < 86400:
        return i18n.t(lang, "age_hour", n=seconds // 3600)
    return i18n.t(lang, "age_day", n=seconds // 86400)


def _state_segment(kind: str, value, *, lang: str) -> str:
    """Localized band label for a reading (e.g. '⚠️ dry'), or '' for kinds
    without bands. Keeps the bot's wording consistent with the web dashboard."""
    tone = sensor_state.tone_for(kind, value)
    if tone is None:
        return ""
    return f"{sensor_state.emoji_for(tone)} {i18n.t(lang, f'band_{tone}')}".strip()


def format_sensor_line(sensor: dict, *, lang: str = i18n.DEFAULT, now: datetime | None = None) -> str:
    kind = sensor.get("kind", "")
    label = sensor.get("friendly_name") or _label(lang, "kind", kind)
    value = sensor.get("last_value")
    if value is None:
        return f"• {escape(label)}: —"
    unit = sensor.get("unit") or ""
    rendered = f"{value:g} {unit}".strip()
    age = humanize_age(sensor.get("last_value_at"), lang=lang, now=now)
    state = _state_segment(kind, value, lang=lang)
    suffix = f" · {state}" if state else ""
    return f"• {escape(label)}: {escape(rendered)}{suffix} ({age})"


def format_device_card(
    device: dict,
    sensors: list[dict],
    *,
    lang: str = i18n.DEFAULT,
    now: datetime | None = None,
    note: str | None = None,
) -> str:
    """A single device screen: status header + sensor readings + a pump hint.
    `note` is an optional banner (e.g. a 'command queued' confirmation) shown
    just under the header. Sections are separated by blank lines."""
    label = device.get("friendly_name") or device.get("device_id") or "?"
    online = bool(device.get("online"))
    state = i18n.t(lang, "state_online" if online else "state_offline")
    seen = humanize_age(device.get("last_seen_at"), lang=lang, now=now)
    sections = [f"🪴 <b>{escape(label)}</b>\n{state} · {seen}"]
    if note:
        sections.append(note)
    if sensors:
        sections.append("\n".join(format_sensor_line(s, lang=lang, now=now) for s in sensors))
    else:
        sections.append(i18n.t(lang, "no_sensors"))
    sections.append(i18n.t(lang, "card_pump_hint"))
    return "\n\n".join(sections)


def format_device_button(device: dict) -> str:
    dot = "🟢" if device.get("online") else "⚪️"
    name = device.get("friendly_name") or device["device_id"]
    return f"{dot} {name}"


def format_event(event: dict, *, lang: str = i18n.DEFAULT, now: datetime | None = None) -> str:
    kind = event.get("kind", "")
    label = _label(lang, "event", kind)
    who = event.get("device_name") or event.get("device_id") or "?"
    age = humanize_age(event.get("ts"), lang=lang, now=now)
    text = f"{escape(label)}\n{i18n.t(lang, 'event_device', name=escape(who))}\n{age}"
    payload = event.get("payload") or {}
    duration = payload.get("duration_ms")
    if duration is not None:
        text += "\n" + i18n.t(lang, "event_duration", sec=f"{duration / 1000:g}")
    return text
