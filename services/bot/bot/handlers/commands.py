"""Slash-command handlers: /start, /menu, /sensors, /language."""
from telegram import Update
from telegram.constants import ParseMode
from telegram.ext import ContextTypes

from bot import i18n
from bot.core import store
from bot.core.auth import is_allowed
from bot.handlers.common import current_language, deny, hub, user_id
from bot.clients.hub import HubError
from bot.ui import keyboards


async def start(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    lang = current_language(update, context)
    if not is_allowed(user_id(update)):
        await deny(update, lang)
        return
    store.add_subscriber(context.application.bot_data, update.effective_chat.id, lang=lang)
    await update.message.reply_text(
        i18n.t(lang, "welcome"), reply_markup=keyboards.main_menu(lang),
        parse_mode=ParseMode.HTML,
    )


async def menu(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    lang = current_language(update, context)
    if not is_allowed(user_id(update)):
        await deny(update, lang)
        return
    await update.message.reply_text(
        i18n.t(lang, "menu_title"), reply_markup=keyboards.main_menu(lang)
    )


async def language(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    lang = current_language(update, context)
    if not is_allowed(user_id(update)):
        await deny(update, lang)
        return
    await update.message.reply_text(
        i18n.t(lang, "language_title"), reply_markup=keyboards.language_picker()
    )


async def sensors(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    lang = current_language(update, context)
    if not is_allowed(user_id(update)):
        await deny(update, lang)
        return
    try:
        devices = await hub(context).list_devices()
    except HubError:
        await update.message.reply_text(i18n.t(lang, "hub_error"))
        return
    if not devices:
        await update.message.reply_text(i18n.t(lang, "no_devices"))
        return
    await update.message.reply_text(
        i18n.t(lang, "pick_device"),
        reply_markup=keyboards.device_picker(devices, prefix="d", lang=lang),
    )
