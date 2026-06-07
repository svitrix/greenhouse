import { describe, expect, it, vi } from 'vitest';
import { render, fireEvent } from '@testing-library/preact';
import { PumpPanel } from './PumpPanel';
import type { PumpView } from '../../api/types';

describe('PumpPanel', () => {
  it('renders state and triggers onTogglePump', () => {
    const onToggle = vi.fn();
    const pump: PumpView = { state: 'OFF', remaining_s: 0, last_run_ms: 0 };
    const { getByText } = render(<PumpPanel pump={pump} onTogglePump={onToggle} />);
    fireEvent.click(getByText(/run 10s/i));
    expect(onToggle).toHaveBeenCalledWith('ON');
  });

  it('shows lockout reason when locked', () => {
    const pump: PumpView = { state: 'LOCKED', remaining_s: 0, last_run_ms: 0,
                              lockout_reason: 'lock:max_runtime' };
    const { getByText } = render(<PumpPanel pump={pump} onTogglePump={() => {}} />);
    expect(getByText(/lock:max_runtime/i)).toBeTruthy();
  });
});
