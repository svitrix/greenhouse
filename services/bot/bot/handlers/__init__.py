"""Telegram update handlers. `register()` wires them onto the Application so
the composition root (app.py) stays agnostic of the individual callbacks."""
from telegram.ext import Application, CallbackQueryHandler, CommandHandler

from bot.handlers import callbacks, commands


def register(application: Application) -> None:
    application.add_handler(CommandHandler("start", commands.start))
    application.add_handler(CommandHandler("menu", commands.menu))
    application.add_handler(CommandHandler("sensors", commands.sensors))
    application.add_handler(CommandHandler("language", commands.language))
    application.add_handler(CallbackQueryHandler(callbacks.on_callback))
