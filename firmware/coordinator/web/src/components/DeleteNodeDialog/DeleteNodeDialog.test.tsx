import { describe, expect, it, vi } from 'vitest';
import { render, fireEvent } from '@testing-library/preact';
import { DeleteNodeDialog } from './DeleteNodeDialog';

describe('DeleteNodeDialog', () => {
  it('requires typed alias before confirming', () => {
    const onConfirm = vi.fn();
    const { getByRole, getByText } = render(
      <DeleteNodeDialog open alias="Tomatoes" short_addr="0x1A2B"
                         onConfirm={onConfirm} onCancel={() => {}} />
    );
    fireEvent.click(getByText(/i understand/i));
    const input = getByRole('textbox');
    fireEvent.input(input, { target: { value: 'wrong' } });
    fireEvent.click(getByText(/forget node/i));
    expect(onConfirm).not.toHaveBeenCalled();

    fireEvent.input(input, { target: { value: 'Tomatoes' } });
    fireEvent.click(getByText(/forget node/i));
    expect(onConfirm).toHaveBeenCalled();
  });

  it('renders nothing when closed', () => {
    const { container } = render(
      <DeleteNodeDialog open={false} alias="x" short_addr="0x1A2B"
                         onConfirm={() => {}} onCancel={() => {}} />
    );
    expect(container.firstChild).toBeNull();
  });
});
