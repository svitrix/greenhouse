import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { renderHook, waitFor } from "@testing-library/react";
import { ReactNode } from "react";
import { useDevicesQuery } from "./useDevicesQuery";
import { useAuthStore } from "@/lib/auth";

function wrapper({ children }: { children: ReactNode }) {
  const qc = new QueryClient({
    defaultOptions: { queries: { retry: false } },
  });
  return <QueryClientProvider client={qc}>{children}</QueryClientProvider>;
}

describe("useDevicesQuery", () => {
  beforeEach(() => {
    useAuthStore.setState({ token: "t", username: "u" });
  });
  afterEach(() => vi.restoreAllMocks());

  it("parses a valid response", async () => {
    vi.spyOn(globalThis, "fetch").mockResolvedValueOnce(
      new Response(
        JSON.stringify([
          {
            device_id: "gh-a",
            friendly_name: null,
            fw_version: "0.4.0",
            profile_id: "gh-coordinator-v1",
            location_id: null,
            last_seen_at: "2026-06-03T10:00:00+00:00",
            sensors_count: 6,
            online: true,
          },
        ]),
        { status: 200 },
      ),
    );
    const { result } = renderHook(() => useDevicesQuery(), { wrapper });
    await waitFor(() => expect(result.current.isSuccess).toBe(true));
    expect(result.current.data).toHaveLength(1);
    expect(result.current.data?.[0].device_id).toBe("gh-a");
    expect(result.current.data?.[0].sensors_count).toBe(6);
  });

  it("surfaces ApiError on 5xx", async () => {
    vi.spyOn(globalThis, "fetch").mockResolvedValueOnce(
      new Response(JSON.stringify({ detail: "boom" }), { status: 500 }),
    );
    const { result } = renderHook(() => useDevicesQuery(), { wrapper });
    await waitFor(() => expect(result.current.isError).toBe(true));
    expect((result.current.error as { status?: number })?.status).toBe(500);
  });
});
