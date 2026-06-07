import pytest
from httpx import ASGITransport, AsyncClient

from app.main import app


@pytest.mark.asyncio
async def test_cors_preflight_includes_authorization():
    async with AsyncClient(
        transport=ASGITransport(app=app), base_url="http://t"
    ) as ac:
        r = await ac.options(
            "/api/devices",
            headers={
                "Origin": "http://localhost:3000",
                "Access-Control-Request-Method": "GET",
                "Access-Control-Request-Headers": "authorization",
            },
        )
    assert r.status_code == 200
    assert r.headers["access-control-allow-origin"] == "http://localhost:3000"
    allowed = r.headers["access-control-allow-headers"].lower()
    assert "authorization" in allowed


@pytest.mark.asyncio
async def test_cors_rejects_unknown_origin():
    async with AsyncClient(
        transport=ASGITransport(app=app), base_url="http://t"
    ) as ac:
        r = await ac.options(
            "/api/devices",
            headers={
                "Origin": "http://evil.example.com",
                "Access-Control-Request-Method": "GET",
            },
        )
    # Starlette's CORS middleware returns 400 for disallowed origin preflights
    assert r.status_code == 400 or "access-control-allow-origin" not in r.headers
