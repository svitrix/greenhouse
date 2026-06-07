import { describe, expect, it } from 'vitest';
import { render } from '@testing-library/preact';
import { ChannelBadge } from './ChannelBadge';

describe('ChannelBadge', () => {
  it('renders formatted value and age', () => {
    const { getByText } = render(
      <ChannelBadge kind="air" quantity="temp_c" value={23.4} unit="°C" age_s={12} />
    );
    expect(getByText('23.4 °C')).toBeTruthy();
    expect(getByText(/12s/)).toBeTruthy();
  });

  it('applies stale class for old readings', () => {
    const { container } = render(
      <ChannelBadge kind="soil1" quantity="moisture_pct" value={42} unit="%" age_s={300} />
    );
    expect(container.firstElementChild?.className).toMatch(/stale/);
  });
});
