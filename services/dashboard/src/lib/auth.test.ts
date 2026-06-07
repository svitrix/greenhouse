import { beforeEach, describe, expect, it } from "vitest";
import { useAuthStore } from "./auth";

describe("auth store", () => {
  beforeEach(() => {
    useAuthStore.setState({ token: null, username: null });
    localStorage.clear();
  });

  it("starts unauthenticated", () => {
    expect(useAuthStore.getState().token).toBeNull();
  });

  it("login() sets token and username", () => {
    useAuthStore.getState().login("deadbeef".repeat(8), "alice");
    expect(useAuthStore.getState().token).toBe("deadbeef".repeat(8));
    expect(useAuthStore.getState().username).toBe("alice");
  });

  it("logout() clears state", () => {
    useAuthStore.getState().login("token123", "bob");
    useAuthStore.getState().logout();
    expect(useAuthStore.getState().token).toBeNull();
  });

  it("persists to localStorage", () => {
    useAuthStore.getState().login("persist-token", "carol");
    const raw = localStorage.getItem("gh-admin-auth");
    expect(raw).toBeTruthy();
    expect(raw).toContain("persist-token");
  });
});
