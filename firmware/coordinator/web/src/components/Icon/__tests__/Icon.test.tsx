import { describe, it, expect } from 'vitest';
import { render } from '@testing-library/preact';
import { Icon } from '../Icon';

describe('Icon', () => {
  it('renders an svg with the given size and href fragment', () => {
    const { container } = render(<Icon id="i-leaf" size={24} />);
    const svg = container.querySelector('svg');
    expect(svg).toBeInTheDocument();
    expect(svg?.getAttribute('width')).toBe('24');
    expect(svg?.getAttribute('height')).toBe('24');
    expect(svg?.querySelector('use')?.getAttribute('href')).toBe('/icons.svg#i-leaf');
  });

  it('defaults size to 18', () => {
    const { container } = render(<Icon id="i-x" />);
    expect(container.querySelector('svg')?.getAttribute('width')).toBe('18');
  });
});
