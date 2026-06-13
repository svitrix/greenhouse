"""Bot-owned state, persisted via python-telegram-bot's PicklePersistence
(application.bot_data). The hub owns devices/commands; subscribers, their
language, and the notification cursor live here.

Each subscriber is stored as ``{chat_id: {"notify": bool, "lang": str}}``."""

_SUBSCRIBERS = "subscribers"
_CURSOR = "event_cursor"


def _subs(bot_data: dict) -> dict:
    return bot_data.setdefault(_SUBSCRIBERS, {})


def _entry(bot_data: dict, chat_id: int) -> dict:
    """Return the (mutable) entry for a chat, migrating any legacy shape
    (an earlier version stored a bare bool) to the dict form in place."""
    subs = _subs(bot_data)
    val = subs.get(chat_id)
    if isinstance(val, bool):
        val = {"notify": val}
    elif not isinstance(val, dict):
        val = {}
    subs[chat_id] = val
    return val


def add_subscriber(bot_data: dict, chat_id: int, *, lang: str | None = None) -> None:
    entry = _entry(bot_data, chat_id)
    entry.setdefault("notify", True)
    if lang is not None:
        entry.setdefault("lang", lang)


def set_notify(bot_data: dict, chat_id: int, notify: bool) -> None:
    _entry(bot_data, chat_id)["notify"] = notify


def is_notifying(bot_data: dict, chat_id: int) -> bool:
    return _entry(bot_data, chat_id).get("notify", True)


def set_language(bot_data: dict, chat_id: int, lang: str) -> None:
    _entry(bot_data, chat_id)["lang"] = lang


def get_language(bot_data: dict, chat_id: int) -> str | None:
    val = _subs(bot_data).get(chat_id)
    if isinstance(val, dict):
        return val.get("lang")
    return None


def notify_targets(bot_data: dict) -> list[int]:
    targets = []
    for chat_id, val in _subs(bot_data).items():
        notify = val.get("notify", True) if isinstance(val, dict) else bool(val)
        if notify:
            targets.append(chat_id)
    return targets


def get_cursor(bot_data: dict) -> str | None:
    return bot_data.get(_CURSOR)


def set_cursor(bot_data: dict, ts: str) -> None:
    bot_data[_CURSOR] = ts
