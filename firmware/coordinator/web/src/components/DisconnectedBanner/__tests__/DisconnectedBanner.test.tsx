import { describe, it, expect, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/preact';
import { DisconnectedBanner } from '../DisconnectedBanner';

describe('DisconnectedBanner', () => {
  it('renders the title and body copy', () => {
    render(<DisconnectedBanner onReload={() => {}} />);
    expect(screen.getByText("Can't reach the coordinator.")).toBeInTheDocument();
  });

  it('calls onReload when CTA is clicked', () => {
    const onReload = vi.fn();
    render(<DisconnectedBanner onReload={onReload} />);
    fireEvent.click(screen.getByRole('button', { name: /try again/i }));
    expect(onReload).toHaveBeenCalledOnce();
  });
});
