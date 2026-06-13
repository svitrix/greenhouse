import logging

from telegram.ext import Application, ApplicationBuilder, PicklePersistence

from bot import handlers
from bot.clients.hub import HubClient
from bot.config import get_settings
from bot.jobs.notifier import notify_job

log = logging.getLogger("greenhouse.bot")


class GreenhouseApplication(Application):
    # Application defines __slots__ and exposes no __dict__, so arbitrary
    # attribute assignment is rejected. Declare a slot for the shared,
    # non-picklable HubClient (bot_data is off-limits — PicklePersistence
    # would try to pickle it).
    __slots__ = ("hub_client",)


async def _post_shutdown(application: Application) -> None:
    hub: HubClient | None = getattr(application, "hub_client", None)
    if hub is not None:
        await hub.aclose()


def build_application() -> Application:
    settings = get_settings()
    persistence = PicklePersistence(filepath=settings.PERSISTENCE_PATH)
    application = (
        ApplicationBuilder()
        .application_class(GreenhouseApplication)
        .token(settings.TELEGRAM_BOT_TOKEN)
        .persistence(persistence)
        .post_shutdown(_post_shutdown)
        .build()
    )
    # Shared, non-picklable resource — attached to the Application (only
    # bot_data/chat_data/user_data are persisted, not arbitrary attributes).
    application.hub_client = HubClient(settings.HUB_API_URL, settings.HUB_ADMIN_TOKEN)

    handlers.register(application)
    application.job_queue.run_repeating(
        notify_job, interval=settings.BOT_POLL_SECONDS, first=5
    )
    return application


def main() -> None:
    settings = get_settings()
    logging.basicConfig(
        level=settings.LOG_LEVEL,
        format="%(asctime)s %(levelname)s %(name)s: %(message)s",
    )
    if not settings.TELEGRAM_BOT_TOKEN:
        raise SystemExit("TELEGRAM_BOT_TOKEN is not set")
    if not settings.HUB_ADMIN_TOKEN:
        raise SystemExit("HUB_ADMIN_TOKEN is not set")
    if not settings.telegram_allowed_user_ids:
        log.warning(
            "TELEGRAM_ALLOWED_USER_IDS is empty — the bot will deny every user. "
            "Set it to a comma-separated list of Telegram user ids."
        )

    log.info("greenhouse telegram bot starting (long-polling) against %s",
             settings.HUB_API_URL)
    application = build_application()
    application.run_polling()


if __name__ == "__main__":
    main()
