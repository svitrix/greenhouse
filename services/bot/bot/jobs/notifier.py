import logging

from telegram.constants import ParseMode
from telegram.ext import ContextTypes

from bot import i18n
from bot.clients.hub import HubError
from bot.core import store
from bot.ui.formatting import format_event

log = logging.getLogger("greenhouse.bot.notifier")

# Event kinds worth a Telegram ping. Mirrors the hub's event vocabulary;
# 'provisioned' and friends stay silent.
NOTIFY_KINDS = ("watered", "dry_run_aborted")


async def notify_job(context: ContextTypes.DEFAULT_TYPE) -> None:
    """JobQueue tick: pull new events from the hub and push them to
    subscribed chats. The cursor (last announced event ts) is persisted in
    bot_data, so a restart neither replays history nor drops events.

    On the very first run the cursor is unset — we pin it to the newest
    event and announce nothing, so the bot doesn't dump the backlog the
    moment it boots."""
    hub = context.application.hub_client  # type: ignore[attr-defined]
    bot_data = context.application.bot_data
    cursor = store.get_cursor(bot_data)

    try:
        events = await hub.list_events(kinds=NOTIFY_KINDS, since=cursor, limit=50)
    except HubError as exc:
        log.warning("event poll failed: %s", exc)
        return
    if not events:
        return

    # The hub returns newest-first; announce in chronological order.
    events.sort(key=lambda e: e["ts"])

    if cursor is None:
        store.set_cursor(bot_data, events[-1]["ts"])
        await context.application.update_persistence()
        return

    targets = store.notify_targets(bot_data)
    for event in events:
        # Render once per language, not once per chat, then fan out.
        rendered: dict[str, str] = {}
        for chat_id in targets:
            lang = store.get_language(bot_data, chat_id) or i18n.DEFAULT
            text = rendered.get(lang)
            if text is None:
                text = rendered[lang] = format_event(event, lang=lang)
            try:
                await context.bot.send_message(
                    chat_id, text, parse_mode=ParseMode.HTML
                )
            except Exception as exc:  # noqa: BLE001 — one bad chat must not stall the rest
                log.warning("notify failed for chat %s: %s", chat_id, exc)

    store.set_cursor(bot_data, events[-1]["ts"])
    await context.application.update_persistence()
