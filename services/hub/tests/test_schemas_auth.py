import pytest
from pydantic import ValidationError

from app.schemas.auth import LoginIn


def test_login_accepts_valid():
    LoginIn(username="admin", password="test1234")


def test_login_rejects_short_password():
    with pytest.raises(ValidationError):
        LoginIn(username="admin", password="short")


def test_login_rejects_empty_username():
    with pytest.raises(ValidationError):
        LoginIn(username="", password="test1234")


def test_login_rejects_extra_fields():
    with pytest.raises(ValidationError):
        LoginIn(username="admin", password="test1234", remember_me=True)
