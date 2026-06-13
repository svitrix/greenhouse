"""Shared helpers for the command and callback handlers — access control,
language resolution, and the hub-client accessor."""
from telegram import Update
from telegram.ext import ContextTypes

from bot import i18n
from bot.clients.hub import HubClient
from bot.core import store


def hub(context: ContextTypes.DEFAULT_TYPE) -> HubClient:
    return context.application.hub_client  # type: ignore[attr-defined]


def user_id(update: Update) -> int | None:
    return update.effective_user.id if update.effective_user else None


def current_language(update: Update, context: ContextTypes.DEFAULT_TYPE) -> str:
    """Stored preference wins; otherwise fall back to the user's Telegram
    client language, then English."""
    chat = update.effective_chat
    stored = store.get_language(context.application.bot_data, chat.id) if chat else None
    if stored:
        return stored
    user = update.effective_user
    return i18n.resolve_language(user.language_code if user else None)


async def deny(update: Update, lang: str) -> None:
    if update.callback_query:
        await update.callback_query.answer(i18n.t(lang, "denied_alert"), show_alert=True)
    elif update.message:
        await update.message.reply_text(i18n.t(lang, "denied"))
