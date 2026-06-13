from collections.abc import Iterable

import httpx


class HubError(Exception):
    """Any non-success response or transport failure from the hub."""


class DeviceNotFound(HubError):
    """The hub returned 404 for the targeted device."""


class HubClient:
    """Thin async wrapper over the hub admin REST API. The bot never touches
    the database directly — everything goes through these endpoints with an
    admin bearer token."""

    def __init__(self, base_url: str, token: str, *, timeout: float = 10.0) -> None:
        self._client = httpx.AsyncClient(
            base_url=base_url.rstrip("/"),
            headers={"Authorization": f"Bearer {token}"},
            timeout=timeout,
        )

    # Allow injecting a pre-built client (tests use httpx.MockTransport).
    @classmethod
    def with_client(cls, client: httpx.AsyncClient) -> "HubClient":
        self = cls.__new__(cls)
        self._client = client
        return self

    async def aclose(self) -> None:
        await self._client.aclose()

    async def list_devices(self) -> list[dict]:
        return await self._get_json("/api/devices")

    async def get_device(self, device_id: str) -> dict | None:
        try:
            return await self._get_json(f"/api/devices/{device_id}")
        except DeviceNotFound:
            return None

    async def list_sensors(self, device_id: str) -> list[dict]:
        return await self._get_json("/api/sensors", params={"device_id": device_id})

    async def list_events(
        self,
        *,
        kinds: Iterable[str] | None = None,
        since: str | None = None,
        limit: int = 50,
    ) -> list[dict]:
        params: dict = {"limit": limit}
        if kinds:
            params["kind"] = list(kinds)
        if since:
            params["since"] = since
        return await self._get_json("/api/events", params=params)

    async def enqueue_command(
        self, device_id: str, command: str, params: dict | None = None,
    ) -> dict:
        try:
            resp = await self._client.post(
                f"/api/devices/{device_id}/commands",
                json={"command": command, "params": params},
            )
        except httpx.HTTPError as exc:
            raise HubError(str(exc)) from exc
        if resp.status_code == 404:
            raise DeviceNotFound(device_id)
        if resp.is_error:
            raise HubError(f"{resp.status_code}: {resp.text}")
        return resp.json()

    async def _get_json(self, path: str, *, params: dict | None = None):
        try:
            resp = await self._client.get(path, params=params)
        except httpx.HTTPError as exc:
            raise HubError(str(exc)) from exc
        if resp.status_code == 404:
            raise DeviceNotFound(path)
        if resp.is_error:
            raise HubError(f"{resp.status_code}: {resp.text}")
        return resp.json()
