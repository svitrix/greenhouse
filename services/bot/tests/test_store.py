from bot.core import store


def test_add_subscriber_sets_notify_and_language():
    bot_data: dict = {}
    store.add_subscriber(bot_data, 42, lang="uk")
    assert store.is_notifying(bot_data, 42) is True
    assert store.get_language(bot_data, 42) == "uk"
    assert store.notify_targets(bot_data) == [42]


def test_add_subscriber_does_not_overwrite_existing_language():
    bot_data: dict = {}
    store.add_subscriber(bot_data, 1, lang="uk")
    store.add_subscriber(bot_data, 1, lang="en")  # second /start
    assert store.get_language(bot_data, 1) == "uk"


def test_set_language_and_notify_toggle():
    bot_data: dict = {}
    store.add_subscriber(bot_data, 7, lang="en")
    store.set_language(bot_data, 7, "uk")
    assert store.get_language(bot_data, 7) == "uk"
    store.set_notify(bot_data, 7, False)
    assert store.is_notifying(bot_data, 7) is False
    assert store.notify_targets(bot_data) == []


def test_legacy_bool_shape_is_migrated():
    # An older build stored a bare bool per chat.
    bot_data: dict = {"subscribers": {99: True}}
    assert store.is_notifying(bot_data, 99) is True
    assert 99 in store.notify_targets(bot_data)
    store.set_language(bot_data, 99, "uk")
    assert store.get_language(bot_data, 99) == "uk"
    assert isinstance(bot_data["subscribers"][99], dict)


def test_cursor_roundtrip():
    bot_data: dict = {}
    assert store.get_cursor(bot_data) is None
    store.set_cursor(bot_data, "2026-06-13T10:00:00+00:00")
    assert store.get_cursor(bot_data) == "2026-06-13T10:00:00+00:00"
