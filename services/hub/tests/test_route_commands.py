import pytest
from httpx import ASGITransport, AsyncClient
from sqlalchemy.ext.asyncio import async_sessionmaker

from app.main import app
from app.tools.mint_admin_token import mint_admin_token


@pytest.fixture
def wired_app(monkeypatch, migrated_db):
    from app import db
    monkeypatch.setattr(db, "_engine", migrated_db)
    monkeypatch.setattr(
        db, "_sessionmaker",
        async_sessionmaker(migrated_db, expire_on_commit=False),
    )
    return app


def _bearer(t):
    return {"Authorization": f"Bearer {t}"}


async def _pair_one(ac, admin: str, device_id: str) -> str:
    r = await ac.post(
        "/api/pairing/open", json={"ttl_seconds": 300}, headers=_bearer(admin)
    )
    r2 = await ac.post(
        "/api/pairing/claim",
        json={
            "claim_code": r.json()["code"],
            "device_id":  device_id,
            "mac":        "aa:bb:cc:dd:ee:ff",
            "fw_version": "0.4.0",
            "profile_id": "gh-coordinator-v1",
        },
    )
    return r2.json()["api_key"]


@pytest.mark.asyncio
async def test_enqueue_poll_ack_roundtrip(wired_app, migrated_db):
    admin = await mint_admin_token(name="cmd-rt")
    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        api_key = await _pair_one(ac, admin, "gh-cmdrt01")

        r = await ac.post(
            "/api/devices/gh-cmdrt01/commands",
            json={"command": "pump_on", "params": {"duration_ms": 4000}},
            headers=_bearer(admin),
        )
        assert r.status_code == 201
        body = r.json()
        cmd_id = body["id"]
        assert body["status"] == "pending"
        assert body["created_by"] == "cmd-rt"

        # Coordinator polls its queue (device bearer).
        rp = await ac.get(
            "/api/devices/gh-cmdrt01/commands/pending", headers=_bearer(api_key)
        )
        assert rp.status_code == 200
        assert [c["id"] for c in rp.json()] == [cmd_id]
        assert rp.json()[0]["command"] == "pump_on"
        assert rp.json()[0]["params"] == {"duration_ms": 4000}

        # Already handed out → empty on the next poll.
        rp2 = await ac.get(
            "/api/devices/gh-cmdrt01/commands/pending", headers=_bearer(api_key)
        )
        assert rp2.json() == []

        ra = await ac.post(
            f"/api/commands/{cmd_id}/ack",
            json={"status": "acked", "result": {"ran_ms": 4000}},
            headers=_bearer(api_key),
        )
        assert ra.status_code == 200
        assert ra.json()["status"] == "acked"

        rl = await ac.get(
            "/api/devices/gh-cmdrt01/commands", headers=_bearer(admin)
        )
        assert rl.json()[0]["status"] == "acked"


@pytest.mark.asyncio
async def test_enqueue_unknown_device_is_404(wired_app, migrated_db):
    admin = await mint_admin_token(name="cmd-404")
    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.post(
            "/api/devices/gh-missing/commands",
            json={"command": "pump_on"},
            headers=_bearer(admin),
        )
        assert r.status_code == 404


@pytest.mark.asyncio
async def test_invalid_command_kind_is_422(wired_app, migrated_db):
    admin = await mint_admin_token(name="cmd-422")
    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        await _pair_one(ac, admin, "gh-cmd42201")
        r = await ac.post(
            "/api/devices/gh-cmd42201/commands",
            json={"command": "self_destruct"},
            headers=_bearer(admin),
        )
        assert r.status_code == 422


@pytest.mark.asyncio
async def test_poll_requires_matching_device(wired_app, migrated_db):
    admin = await mint_admin_token(name="cmd-mismatch")
    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        key_a = await _pair_one(ac, admin, "gh-cmdmm-a")
        await _pair_one(ac, admin, "gh-cmdmm-b")
        # Device A's key may not poll device B's queue.
        r = await ac.get(
            "/api/devices/gh-cmdmm-b/commands/pending", headers=_bearer(key_a)
        )
        assert r.status_code == 403


@pytest.mark.asyncio
async def test_enqueue_requires_admin(wired_app, migrated_db):
    async with AsyncClient(
        transport=ASGITransport(app=wired_app), base_url="http://t"
    ) as ac:
        r = await ac.post(
            "/api/devices/gh-x/commands", json={"command": "pump_on"}
        )
        assert r.status_code == 401
