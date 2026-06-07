import { describe, expect, it, vi, beforeEach } from "vitest";
import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { MemoryRouter } from "react-router-dom";
import Login from "./Login";
import { useAuthStore } from "@/lib/auth";

function renderLogin() {
  const qc = new QueryClient({ defaultOptions: { queries: { retry: false } } });
  return render(
    <QueryClientProvider client={qc}>
      <MemoryRouter>
        <Login />
      </MemoryRouter>
    </QueryClientProvider>,
  );
}

describe("<Login />", () => {
  beforeEach(() => {
    useAuthStore.setState({ token: null, username: null });
    vi.restoreAllMocks();
  });

  it("renders the form", () => {
    renderLogin();
    expect(screen.getByLabelText(/username/i)).toBeInTheDocument();
    expect(screen.getByLabelText(/password/i)).toBeInTheDocument();
  });

  it("validates min password length before submit", async () => {
    renderLogin();
    const user = userEvent.setup();
    await user.type(screen.getByLabelText(/username/i), "admin");
    await user.type(screen.getByLabelText(/password/i), "short");
    await user.click(screen.getByRole("button", { name: /sign in/i }));
    expect(
      await screen.findByText(/at least 8 characters/i),
    ).toBeInTheDocument();
  });

  it("calls login and navigates on 200", async () => {
    vi.spyOn(globalThis, "fetch").mockResolvedValueOnce(
      new Response(
        JSON.stringify({ admin_token: "a".repeat(64), name: "login-admin-x" }),
        { status: 200 },
      ),
    );
    renderLogin();
    const user = userEvent.setup();
    await user.type(screen.getByLabelText(/username/i), "admin");
    await user.type(screen.getByLabelText(/password/i), "test1234");
    await user.click(screen.getByRole("button", { name: /sign in/i }));

    // store should be populated within microtasks
    await vi.waitUntil(() => useAuthStore.getState().token !== null);
    expect(useAuthStore.getState().token).toBe("a".repeat(64));
    expect(useAuthStore.getState().username).toBe("admin");
  });
});
