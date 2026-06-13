"""Inline-button callback router and its per-screen handlers."""
import logging

from telegram import Update
from telegram.constants import ParseMode
from telegram.ext import ContextTypes

from bot import i18n
from bot.clients.hub import DeviceNotFound, HubError
from bot.core import store
from bot.core.auth import is_allowed
from bot.handlers.common import current_language, deny, hub, user_id
from bot.ui import keyboards
from bot.ui.formatting import format_device_card

log = logging.getLogger("greenhouse.bot")


async def on_callback(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    lang = current_language(update, context)
    if not is_allowed(user_id(update)):
        await deny(update, lang)
        return
    query = update.callback_query
    await query.answer()
    data = query.data or ""

    try:
        if data == "m:main":
            await query.edit_message_text(
                i18n.t(lang, "menu_title"), reply_markup=keyboards.main_menu(lang)
            )
        elif data == "m:devices":
            await _device_picker(context, query, lang, prefix="d", title_key="pick_device")
        elif data == "m:notify":
            await _show_notify(context, query, lang)
        elif data == "m:lang":
            await query.edit_message_text(
                i18n.t(lang, "language_title"), reply_markup=keyboards.language_picker()
            )
        elif data.startswith("lang:"):
            await _set_language(context, query, data[5:])
        elif data.startswith("po:"):
            await _pump_cmd(context, update, query, lang, data[3:], "pump_on")
        elif data.startswith("pf:"):
            await _pump_cmd(context, update, query, lang, data[3:], "pump_off")
        elif data.startswith("d:"):
            await _show_card(context, query, lang, data[2:])
        elif data == "n:on":
            await _set_notify(context, query, lang, True)
        elif data == "n:off":
            await _set_notify(context, query, lang, False)
    except HubError:
        await query.edit_message_text(
            i18n.t(lang, "hub_error"), reply_markup=keyboards.back_to_main(lang)
        )


async def _device_picker(context, query, lang: str, *, prefix: str, title_key: str) -> None:
    devices = await hub(context).list_devices()
    if not devices:
        await query.edit_message_text(
            i18n.t(lang, "no_devices"), reply_markup=keyboards.back_to_main(lang)
        )
        return
    await query.edit_message_text(
        i18n.t(lang, title_key),
        reply_markup=keyboards.device_picker(devices, prefix=prefix, lang=lang),
    )


async def _show_card(context, query, lang: str, device_id: str, *, note: str | None = None) -> None:
    client = hub(context)
    device = await client.get_device(device_id)
    if device is None:
        await query.edit_message_text(
            i18n.t(lang, "device_not_found"), reply_markup=keyboards.back_to_main(lang)
        )
        return
    sensors = await client.list_sensors(device_id)
    await query.edit_message_text(
        format_device_card(device, sensors, lang=lang, note=note),
        reply_markup=keyboards.device_card(device_id, lang),
        parse_mode=ParseMode.HTML,
    )


async def _pump_cmd(context, update: Update, query, lang: str, device_id: str, command: str) -> None:
    user = update.effective_user
    try:
        await hub(context).enqueue_command(device_id, command)
    except DeviceNotFound:
        await query.edit_message_text(
            i18n.t(lang, "device_not_found"), reply_markup=keyboards.back_to_main(lang)
        )
        return
    who = f" ({user.username})" if user and user.username else ""
    log.info("pump command %s queued for %s by %s", command, device_id, user_id(update))
    key = "pump_queued_on" if command == "pump_on" else "pump_queued_off"
    note = i18n.t(lang, key, device=device_id, who=who)
    await _show_card(context, query, lang, device_id, note=note)


async def _show_notify(context, query, lang: str) -> None:
    enabled = store.is_notifying(context.application.bot_data, query.message.chat_id)
    word = i18n.t(lang, "notify_on_word" if enabled else "notify_off_word")
    await query.edit_message_text(
        i18n.t(lang, "notify_state", state=word),
        reply_markup=keyboards.notify_toggle(enabled, lang),
        parse_mode=ParseMode.HTML,
    )


async def _set_notify(context, query, lang: str, enabled: bool) -> None:
    store.set_notify(context.application.bot_data, query.message.chat_id, enabled)
    await _show_notify(context, query, lang)


async def _set_language(context, query, lang: str) -> None:
    if lang not in i18n.SUPPORTED:
        lang = i18n.DEFAULT
    store.set_language(context.application.bot_data, query.message.chat_id, lang)
    await query.edit_message_text(
        i18n.t(lang, "language_set"), reply_markup=keyboards.main_menu(lang)
    )
