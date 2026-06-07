import { describe, expect, it, vi } from "vitest";
import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import type { Device } from "@/schemas/device";
import { DeviceTable } from "./DeviceTable";

const sample: Device[] = [
  {
    device_id: "gh-a1b2c3",
    friendly_name: "Greenhouse 1",
    fw_version: "0.4.0",
    profile_id: "gh-coordinator-v1",
    location_id: null,
    last_seen_at: new Date(Date.now() - 60_000).toISOString(),
    sensors_count: 6,
    online: true,
  },
  {
    device_id: "gh-deadbe",
    friendly_name: null,
    fw_version: "0.4.0",
    profile_id: "gh-coordinator-v1",
    location_id: null,
    last_seen_at: new Date(Date.now() - 86_400_000).toISOString(),
    sensors_count: 6,
    online: false,
  },
];

describe("<DeviceTable />", () => {
  it("renders one row per device", () => {
    render(
      <DeviceTable
        devices={sample}
        onRevoke={() => {}}
        onShowDetail={() => {}}
      />,
    );
    expect(screen.getByText("gh-a1b2c3")).toBeInTheDocument();
    expect(screen.getByText("gh-deadbe")).toBeInTheDocument();
  });

  it("shows an em-dash for missing friendly_name", () => {
    render(
      <DeviceTable
        devices={sample}
        onRevoke={() => {}}
        onShowDetail={() => {}}
      />,
    );
    expect(screen.getByText("—")).toBeInTheDocument();
  });

  it("calls onRevoke when 'Revoke credential' is clicked", async () => {
    const onRevoke = vi.fn();
    render(
      <DeviceTable
        devices={sample}
        onRevoke={onRevoke}
        onShowDetail={() => {}}
      />,
    );
    const user = userEvent.setup();
    const triggers = screen.getAllByLabelText("row actions");
    await user.click(triggers[0]);
    await user.click(await screen.findByText(/revoke credential/i));
    expect(onRevoke).toHaveBeenCalledWith("gh-a1b2c3");
  });

  it("renders empty state when no devices", () => {
    render(
      <DeviceTable devices={[]} onRevoke={() => {}} onShowDetail={() => {}} />,
    );
    expect(screen.getByText(/no devices yet/i)).toBeInTheDocument();
  });
});
