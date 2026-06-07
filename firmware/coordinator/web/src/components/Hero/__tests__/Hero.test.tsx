import { describe, it, expect } from 'vitest';
import { render, screen } from '@testing-library/preact';
import { Hero } from '../Hero';

describe('Hero', () => {
  it('renders a default greeting based on the hour', () => {
    render(<Hero />);
    expect(screen.getByText(/updated just now/)).toBeInTheDocument();
  });

  it('renders the override greeting and updated text', () => {
    render(<Hero greetingOverride="Custom hello" updatedText="moments ago" />);
    expect(screen.getByText('Custom hello')).toBeInTheDocument();
    expect(screen.getByText(/moments ago/)).toBeInTheDocument();
  });
});
