from typing import Annotated

from pydantic import BaseModel, ConfigDict, Field


class LoginIn(BaseModel):
    model_config = ConfigDict(extra="forbid")
    username: Annotated[str, Field(min_length=1, max_length=64)]
    password: Annotated[str, Field(min_length=8, max_length=128)]


class LoginOut(BaseModel):
    admin_token: str
    name: str
