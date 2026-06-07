import { describe, expect, it, vi } from 'vitest';
import { render, fireEvent } from '@testing-library/preact';
import { PermitJoinModal } from './PermitJoinModal';

describe('PermitJoinModal', () => {
  it('calls onOpenWindow with chosen duration', () => {
    const onOpen = vi.fn();
    const { getByRole, getByText } = render(
      <PermitJoinModal open onOpenWindow={onOpen} onClose={() => {}} />
    );
    const slider = getByRole('slider');
    fireEvent.input(slider, { target: { value: '90' } });
    fireEvent.click(getByText(/open network/i));
    expect(onOpen).toHaveBeenCalledWith(90);
  });

  it('renders nothing when closed', () => {
    const { container } = render(
      <PermitJoinModal open={false} onOpenWindow={() => {}} onClose={() => {}} />
    );
    expect(container.firstChild).toBeNull();
  });
});
