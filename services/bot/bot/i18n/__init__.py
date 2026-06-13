"""Tiny JSON-backed i18n. One catalog file per language under
``bot/i18n/locales/<lang>.json``; loaded once at import via importlib.resources
(works both from source and an installed package). `t(lang, key, **kwargs)`
formats a template; unknown keys fall back to English, then to the raw key,
so a missing translation degrades loudly-but-safely instead of crashing.

To add a language: drop ``locales/<code>.json`` with the full key set, add
the code to ``SUPPORTED``, and give it a ``language_name`` key. The
``test_every_key_exists_in_all_languages`` test enforces key parity."""
import json
from importlib.resources import files

SUPPORTED = ("en", "uk")
DEFAULT = "en"


def _load(lang: str) -> dict[str, str]:
    raw = (files(__package__) / "locales" / f"{lang}.json").read_text(encoding="utf-8")
    return json.loads(raw)


_CATALOG: dict[str, dict[str, str]] = {lang: _load(lang) for lang in SUPPORTED}

# Each locale self-describes its display name (with flag) under language_name.
LANGUAGE_NAMES = {lang: _CATALOG[lang]["language_name"] for lang in SUPPORTED}


def resolve_language(code: str | None) -> str:
    """Map a Telegram `language_code` (e.g. 'uk', 'en-US') to a supported
    language, defaulting to English."""
    if not code:
        return DEFAULT
    base = code.split("-")[0].lower()
    return base if base in SUPPORTED else DEFAULT


def t(lang: str, key: str, **kwargs) -> str:
    table = _CATALOG.get(lang, _CATALOG[DEFAULT])
    template = table.get(key)
    if template is None:
        template = _CATALOG[DEFAULT].get(key, key)
    return template.format(**kwargs) if kwargs else template
