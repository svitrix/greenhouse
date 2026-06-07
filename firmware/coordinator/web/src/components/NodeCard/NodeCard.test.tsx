import { describe, expect, it } from 'vitest';
import { render } from '@testing-library/preact';
import { NodeCard } from './NodeCard';
import type { NodeView } from '../../api/types';

const node: NodeView = {
  ieee: '00124B001A2B3C4D', short_addr: '0x1A2B', alias: 'T',
  online: true, last_seen_s: 1, rssi_dbm: -50,
  proto_version: 1, proto_version_mismatch: false, present_mask: '0x07',
  readings: [
    { kind: 'air',     quantity: 'temp_c',       value: 22, unit: '°C', age_s: 1 },
    { kind: 'soil1',   quantity: 'moisture_pct', value: 40, unit: '%',  age_s: 1 },
    { kind: 'battery', quantity: 'pct',          value: 80, unit: '%',  age_s: 1 },
  ],
};

describe('NodeCard', () => {
  it('renders one ChannelBadge per reading', () => {
    const { container } = render(<NodeCard node={node} onAliasChange={() => {}} />);
    expect(container.querySelectorAll('[data-kind]').length).toBe(3);
  });

  it('dims when offline', () => {
    const { container } = render(
      <NodeCard node={{ ...node, online: false }} onAliasChange={() => {}} />
    );
    expect(container.firstElementChild?.className).toMatch(/offline/);
  });
});
