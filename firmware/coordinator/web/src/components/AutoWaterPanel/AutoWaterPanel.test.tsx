import { describe, expect, it } from 'vitest';
import { render } from '@testing-library/preact';
import { AutoWaterPanel } from './AutoWaterPanel';
import type { AutoWaterState } from '../../api/types';

describe('AutoWaterPanel', () => {
  it('shows avg moisture and source counts', () => {
    const state: AutoWaterState = {
      avg_moisture_pct: 40.5,
      fresh_sources: ['00AA', '00BB'],
      stale_sources: ['00CC'],
      last_decision_ms: 1000,
      last_decision: 'skip:above_threshold',
    };
    const { getByText } = render(<AutoWaterPanel state={state} />);
    expect(getByText(/41%/)).toBeTruthy();
    expect(getByText(/2 fresh/)).toBeTruthy();
    expect(getByText(/1 stale/)).toBeTruthy();
    expect(getByText(/skip:above_threshold/)).toBeTruthy();
  });

  it('renders em-dash when avg is null', () => {
    const state: AutoWaterState = {
      avg_moisture_pct: null,
      fresh_sources: [], stale_sources: [],
      last_decision_ms: 0, last_decision: 'lock:no_fresh_soil',
    };
    const { getByText } = render(<AutoWaterPanel state={state} />);
    expect(getByText(/—/)).toBeTruthy();
  });
});
