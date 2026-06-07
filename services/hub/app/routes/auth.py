import hashlib
import secrets
from datetime import datetime, timezone

import argon2
from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy import func, select
from sqlalchemy.dialects.postgresql import insert as pg_insert
from sqlalchemy.ext.asyncio import AsyncSession

from app.db import get_session
from app.models import AdminToken, AdminUser
from app.schemas.auth import LoginIn, LoginOut


router = APIRouter(prefix="/api/auth", tags=["auth"])


# Pre-computed hash of the empty string used for constant-time mitigation
# when the username is unknown. The verify call always runs; the result is
# discarded. This keeps the HTTP-timing signal flat across user-exists vs
# user-missing.
_DUMMY_HASH = argon2.PasswordHasher().hash("__never_a_real_password__")


@router.post("/login", response_model=LoginOut)
async def login(
    body: LoginIn,
    session: AsyncSession = Depends(get_session),
) -> LoginOut:
    row = (
        await session.execute(
            select(AdminUser.password_hash).where(AdminUser.username == body.username)
        )
    ).scalar_one_or_none()

    hasher = argon2.PasswordHasher()
    try:
        hasher.verify(row if row is not None else _DUMMY_HASH, body.password)
        if row is None:
            # We verified successfully against the dummy — that can only mean
            # `body.password == "__never_a_real_password__"`, which is still
            # an unknown user, so reject.
            raise HTTPException(401, detail="invalid username or password")
    except argon2.exceptions.VerifyMismatchError:
        raise HTTPException(401, detail="invalid username or password")

    token = secrets.token_hex(32)
    today = datetime.now(tz=timezone.utc).date().isoformat()
    name = f"login-{body.username}-{today}-{secrets.token_hex(2)}"
    await session.execute(
        pg_insert(AdminToken).values(
            token_hash=hashlib.sha256(token.encode()).hexdigest(),
            name=name,
            created_at=func.now(),
        )
    )
    await session.commit()
    return LoginOut(admin_token=token, name=name)
