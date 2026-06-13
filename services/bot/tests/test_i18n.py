from bot import i18n


def test_resolve_language_maps_supported():
    assert i18n.resolve_language("uk") == "uk"
    assert i18n.resolve_language("uk-UA") == "uk"
    assert i18n.resolve_language("en-US") == "en"


def test_resolve_language_defaults_for_unsupported_or_missing():
    assert i18n.resolve_language("ru") == "en"
    assert i18n.resolve_language(None) == "en"
    assert i18n.resolve_language("") == "en"


def test_t_returns_localised_string():
    assert i18n.t("uk", "menu_title") == "Головне меню:"
    assert i18n.t("en", "menu_title") == "Main menu:"


def test_t_falls_back_to_english_then_key():
    # Unknown key → returns the key itself, in either language.
    assert i18n.t("en", "totally_missing") == "totally_missing"
    assert i18n.t("uk", "totally_missing") == "totally_missing"


def test_t_formats_kwargs():
    out = i18n.t("en", "pump_queued_on", device="gh-1", who="")
    assert "gh-1" in out
    assert "{" not in out


def test_unknown_language_falls_back_to_english():
    assert i18n.t("de", "menu_title") == "Main menu:"


def test_every_key_exists_in_all_languages():
    en_keys = set(i18n._CATALOG["en"])
    for lang in i18n.SUPPORTED:
        assert set(i18n._CATALOG[lang]) == en_keys, f"{lang} catalog mismatch"
