import json
from functools import lru_cache

from pydantic import Field
from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    model_config = SettingsConfigDict(env_file=".env", case_sensitive=True)

    DB_URL: str = "postgresql+asyncpg://postgres:dev@localhost:5432/greenhouse"
    DEVICE_API_KEYS_RAW: str = Field(default="{}", alias="DEVICE_API_KEYS")
    LOG_LEVEL: str = "INFO"
    TS_TOLERANCE_HOURS: int = 24
    MAX_BODY_BYTES: int = 1_048_576  # 1 MiB

    @property
    def device_api_keys(self) -> dict[str, str]:
        return json.loads(self.DEVICE_API_KEYS_RAW)


@lru_cache
def get_settings() -> Settings:
    return Settings()  # type: ignore[call-arg]
