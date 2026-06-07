import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { ApiError, apiFetch } from "./api";
import { useAuthStore } from "./auth";

describe("apiFetch", () => {
  // vi.spyOn return type drifts with TS 5.6; `any` keeps the test typing
  // out of the way of the runtime behaviour we're actually verifying.
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  let fetchSpy: any;

  beforeEach(() => {
    useAuthStore.setState({ token: null, username: null });
    fetchSpy = vi.spyOn(globalThis, "fetch");
  });
  afterEach(() => {
    fetchSpy.mockRestore();
  });

  it("adds Authorization header when token present", async () => {
    useAuthStore.getState().login("abc123", "alice");
    fetchSpy.mockResolvedValueOnce(
      new Response(JSON.stringify({ ok: true }), { status: 200 }),
    );
    await apiFetch("/api/devices");
    const headers = (fetchSpy.mock.calls[0][1] as RequestInit).headers as Headers;
    expect(headers.get("Authorization")).toBe("Bearer abc123");
  });

  it("does not add Authorization header when no token", async () => {
    fetchSpy.mockResolvedValueOnce(
      new Response(JSON.stringify({ ok: true }), { status: 200 }),
    );
    await apiFetch("/api/auth/login", {
      method: "POST",
      body: JSON.stringify({}),
    });
    const headers = (fetchSpy.mock.calls[0][1] as RequestInit).headers as Headers;
    expect(headers.get("Authorization")).toBeNull();
  });

  it("logs out on 401", async () => {
    useAuthStore.getState().login("stale-token", "alice");
    fetchSpy.mockResolvedValueOnce(new Response(null, { status: 401 }));
    await expect(apiFetch("/api/devices")).rejects.toBeInstanceOf(ApiError);
    expect(useAuthStore.getState().token).toBeNull();
  });

  it("parses error detail from JSON body on 4xx/5xx", async () => {
    fetchSpy.mockResolvedValueOnce(
      new Response(JSON.stringify({ detail: "bad request" }), { status: 400 }),
    );
    await expect(apiFetch("/api/devices")).rejects.toMatchObject({
      status: 400,
      detail: "bad request",
    });
  });

  it("returns undefined for 204", async () => {
    useAuthStore.getState().login("t", "u");
    fetchSpy.mockResolvedValueOnce(new Response(null, { status: 204 }));
    const result = await apiFetch("/api/devices/x/revoke", { method: "POST" });
    expect(result).toBeUndefined();
  });
});
