import { describe, it, expect } from 'vitest';
import { render, screen } from '@testing-library/preact';
import { Skeleton } from '../Skeleton';

describe('Skeleton', () => {
  it('renders the label and icon', () => {
    const { container } = render(<Skeleton label="Air · temperature" iconId="i-thermometer" />);
    expect(screen.getByText('Air · temperature')).toBeInTheDocument();
    expect(container.querySelector('svg use')?.getAttribute('href')).toBe('/icons.svg#i-thermometer');
  });
});
