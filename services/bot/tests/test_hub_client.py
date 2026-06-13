import json

import httpx
import pytest

from bot.clients.hub import DeviceNotFound, HubClient, HubError


def _client(handler) -> HubClient:
    transport = httpx.MockTransport(handler)
    return HubClient.with_client(
        httpx.AsyncClient(transport=transport, base_url="http://hub")
    )


@pytest.mark.asyncio
async def test_list_devices_returns_payload():
    def handler(request: httpx.Request) -> httpx.Response:
        assert request.url.path == "/api/devices"
        return httpx.Response(200, json=[{"device_id": "gh-1"}])

    hub = _client(handler)
    try:
        assert await hub.list_devices() == [{"device_id": "gh-1"}]
    finally:
        await hub.aclose()


@pytest.mark.asyncio
async def test_get_device_404_returns_none():
    def handler(request: httpx.Request) -> httpx.Response:
        return httpx.Response(404, json={"detail": "not found"})

    hub = _client(handler)
    try:
        assert await hub.get_device("gh-x") is None
    finally:
        await hub.aclose()


@pytest.mark.asyncio
async def test_enqueue_command_posts_body():
    captured = {}

    def handler(request: httpx.Request) -> httpx.Response:
        assert request.method == "POST"
        assert request.url.path == "/api/devices/gh-1/commands"
        captured["body"] = json.loads(request.content)
        return httpx.Response(201, json={"id": "01ABC", "status": "pending"})

    hub = _client(handler)
    try:
        out = await hub.enqueue_command("gh-1", "pump_on")
        assert out["status"] == "pending"
        assert captured["body"] == {"command": "pump_on", "params": None}
    finally:
        await hub.aclose()


@pytest.mark.asyncio
async def test_enqueue_unknown_device_raises():
    def handler(request: httpx.Request) -> httpx.Response:
        return httpx.Response(404, json={"detail": "device not found"})

    hub = _client(handler)
    try:
        with pytest.raises(DeviceNotFound):
            await hub.enqueue_command("gh-missing", "pump_off")
    finally:
        await hub.aclose()


@pytest.mark.asyncio
async def test_list_events_forwards_kind_and_since():
    captured = {}

    def handler(request: httpx.Request) -> httpx.Response:
        captured["params"] = request.url.params
        return httpx.Response(200, json=[])

    hub = _client(handler)
    try:
        await hub.list_events(
            kinds=("watered", "dry_run_aborted"),
            since="2026-06-13T10:00:00+00:00",
            limit=25,
        )
    finally:
        await hub.aclose()
    params = captured["params"]
    assert params.get_list("kind") == ["watered", "dry_run_aborted"]
    assert params["since"] == "2026-06-13T10:00:00+00:00"
    assert params["limit"] == "25"


@pytest.mark.asyncio
async def test_server_error_raises_hub_error():
    def handler(request: httpx.Request) -> httpx.Response:
        return httpx.Response(500, text="boom")

    hub = _client(handler)
    try:
        with pytest.raises(HubError):
            await hub.list_devices()
    finally:
        await hub.aclose()
