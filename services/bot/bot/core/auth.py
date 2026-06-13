from bot.config import get_settings


def is_allowed(user_id: int | None) -> bool:
    """Allowlist gate. An empty allowlist denies everyone — the bot must be
    explicitly told which Telegram user ids may drive it
    (TELEGRAM_ALLOWED_USER_IDS). Fail-closed: a misconfigured bot exposes
    nothing rather than the whole greenhouse."""
    if user_id is None:
        return False
    return user_id in get_settings().telegram_allowed_user_ids
