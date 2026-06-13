from bot.config import get_settings
from bot.core.auth import is_allowed


def test_allowlist_permits_listed_denies_others(monkeypatch):
    monkeypatch.setenv("TELEGRAM_ALLOWED_USER_IDS", "111, 222")
    get_settings.cache_clear()
    try:
        assert is_allowed(111)
        assert is_allowed(222)
        assert not is_allowed(333)
        assert not is_allowed(None)
    finally:
        get_settings.cache_clear()


def test_empty_allowlist_denies_everyone(monkeypatch):
    monkeypatch.setenv("TELEGRAM_ALLOWED_USER_IDS", "")
    get_settings.cache_clear()
    try:
        assert not is_allowed(111)
    finally:
        get_settings.cache_clear()
