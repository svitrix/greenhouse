import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, waitFor } from '@testing-library/preact';

type FetchHandler = (url: string) => Response | Promise<Response>;
let fetchHandler: FetchHandler;

beforeEach(() => {
  fetchHandler = (url) => {
    if (url.endsWith('/scan')) {
      return new Response(JSON.stringify({ networks: [] }),
        { status: 200, headers: { 'content-type': 'application/json' } });
    }
    return new Response('not found', { status: 404 });
  };
  vi.stubGlobal('fetch', vi.fn(async (input: RequestInfo) =>
    fetchHandler(String(input))));
});

import { Provisioning } from '../Provisioning';

describe('Provisioning', () => {
  it('renders title and form sections', () => {
    render(<Provisioning />);
    expect(screen.getByText('Welcome to your greenhouse.')).toBeInTheDocument();
    expect(screen.getByText('Home Wi-Fi')).toBeInTheDocument();
    expect(screen.getAllByText(/Home Assistant/).length).toBeGreaterThan(0);
    expect(screen.getByText('Soil calibration')).toBeInTheDocument();
  });

  it('renders last-connect-error banner when /api/status reports auth_fail', async () => {
    fetchHandler = (url) => {
      if (url.endsWith('/api/status')) {
        return new Response(JSON.stringify({
          device_id: 'greenhouse_a1b2c3',
          uptime_s: 12,
          firmware_version: '0.4.0',
          ip: '192.168.4.1',
          mode: 'provisioning',
          last_connect_error: 'auth_fail',
        }), { status: 200, headers: { 'content-type': 'application/json' } });
      }
      if (url.endsWith('/scan')) {
        return new Response(JSON.stringify({ networks: [] }),
          { status: 200, headers: { 'content-type': 'application/json' } });
      }
      return new Response('not found', { status: 404 });
    };
    render(<Provisioning />);
    await waitFor(() => {
      expect(screen.getByRole('alert')).toHaveTextContent(/wrong password/);
    });
  });

  it('does not render banner when last_connect_error is "none"', async () => {
    fetchHandler = (url) => {
      if (url.endsWith('/api/status')) {
        return new Response(JSON.stringify({
          device_id: 'greenhouse_a1b2c3',
          uptime_s: 12,
          firmware_version: '0.4.0',
          ip: '192.168.4.1',
          mode: 'provisioning',
          last_connect_error: 'none',
        }), { status: 200, headers: { 'content-type': 'application/json' } });
      }
      if (url.endsWith('/scan')) {
        return new Response(JSON.stringify({ networks: [] }),
          { status: 200, headers: { 'content-type': 'application/json' } });
      }
      return new Response('not found', { status: 404 });
    };
    render(<Provisioning />);
    expect(screen.getByText('Welcome to your greenhouse.')).toBeInTheDocument();
    // role="alert" must not appear
    expect(screen.queryByRole('alert')).toBeNull();
  });
});
