from functools import lru_cache

from pydantic import Field
from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    model_config = SettingsConfigDict(env_file=".env", case_sensitive=True)

    TELEGRAM_BOT_TOKEN: str = ""
    TELEGRAM_ALLOWED_USER_IDS_RAW: str = Field(
        default="", alias="TELEGRAM_ALLOWED_USER_IDS"
    )

    HUB_API_URL: str = "http://localhost:8000"
    HUB_ADMIN_TOKEN: str = ""

    BOT_POLL_SECONDS: int = 30
    PERSISTENCE_PATH: str = "/data/bot_persistence.pkl"
    LOG_LEVEL: str = "INFO"

    @property
    def telegram_allowed_user_ids(self) -> set[int]:
        raw = self.TELEGRAM_ALLOWED_USER_IDS_RAW.strip()
        if not raw:
            return set()
        return {int(part) for part in raw.split(",") if part.strip()}


@lru_cache
def get_settings() -> Settings:
    return Settings()  # type: ignore[call-arg]
