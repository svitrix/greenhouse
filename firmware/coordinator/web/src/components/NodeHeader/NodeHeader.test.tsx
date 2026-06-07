import { describe, expect, it, vi } from 'vitest';
import { render, fireEvent } from '@testing-library/preact';
import { NodeHeader } from './NodeHeader';

describe('NodeHeader', () => {
  it('renders alias when present', () => {
    const { getByText } = render(
      <NodeHeader ieee="00124B001A2B3C4D" short_addr="0x1A2B" alias="Tomatoes"
                  online last_seen_s={12} rssi_dbm={-52} onAliasChange={() => {}} />
    );
    expect(getByText('Tomatoes')).toBeTruthy();
  });

  it('falls back to Node 0x… without alias', () => {
    const { getByText } = render(
      <NodeHeader ieee="00124B001A2B3C4D" short_addr="0x1A2B" alias={null}
                  online last_seen_s={12} rssi_dbm={-52} onAliasChange={() => {}} />
    );
    expect(getByText(/Node 0x1A2B/)).toBeTruthy();
  });

  it('calls onAliasChange on Enter', () => {
    const onChange = vi.fn();
    const { getByRole, getByLabelText } = render(
      <NodeHeader ieee="00124B001A2B3C4D" short_addr="0x1A2B" alias="x"
                  online last_seen_s={12} rssi_dbm={-52} onAliasChange={onChange} />
    );
    fireEvent.click(getByLabelText(/edit alias/i));
    const input = getByRole('textbox');
    fireEvent.input(input, { target: { value: 'NewName' } });
    fireEvent.keyDown(input, { key: 'Enter' });
    expect(onChange).toHaveBeenCalledWith('NewName');
  });
});
