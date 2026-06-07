import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, fireEvent, waitFor } from '@testing-library/preact';
import { WifiPicker } from '../WifiPicker';

describe('WifiPicker', () => {
  beforeEach(() => {
    vi.stubGlobal('fetch', vi.fn(async () => new Response(
      JSON.stringify({ networks: [
        { ssid: 'HomeWiFi', rssi: -45 },
        { ssid: 'Neighbor', rssi: -78 },
      ] }),
      { status: 200, headers: { 'content-type': 'application/json' } },
    )));
  });

  it('lists scanned networks and calls onPick when a row is clicked', async () => {
    const onPick = vi.fn();
    render(<WifiPicker selectedSsid="" onPick={onPick} />);
    await waitFor(() => expect(screen.getByText('HomeWiFi')).toBeInTheDocument());
    fireEvent.click(screen.getByRole('button', { name: /HomeWiFi/ }));
    expect(onPick).toHaveBeenCalledWith('HomeWiFi');
  });

  it('shows error message when scan fails', async () => {
    vi.stubGlobal('fetch', vi.fn(async () => new Response('boom', { status: 500 })));
    render(<WifiPicker selectedSsid="" onPick={() => {}} />);
    await waitFor(() => expect(screen.getByText(/Scan failed/)).toBeInTheDocument());
  });
});
