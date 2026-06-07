import { describe, expect, it } from 'vitest';
import { render } from '@testing-library/preact';
import { HistoryChart } from './HistoryChart';

describe('HistoryChart', () => {
  it('renders an SVG path with N segments', () => {
    const points = [
      { ts_ms: 0,    value: 10 },
      { ts_ms: 1000, value: 20 },
      { ts_ms: 2000, value: 15 },
    ];
    const { container } = render(<HistoryChart points={points} width={300} height={80} />);
    const path = container.querySelector('path');
    expect(path).toBeTruthy();
    expect(path?.getAttribute('d')).toMatch(/^M[\s\d.,LMl-]+$/);
  });

  it('renders no data placeholder', () => {
    const { container } = render(<HistoryChart points={[]} width={300} height={80} />);
    expect(container.textContent).toMatch(/no data/i);
  });
});
