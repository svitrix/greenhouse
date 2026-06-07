import { describe, it, expect, beforeEach } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/preact';
import { Login } from '../Login';
import { getCreds, clearCreds, needsLogin, loginError } from '../../../api/auth';

beforeEach(() => {
  sessionStorage.clear();
  clearCreds();
  needsLogin.value = true;
  loginError.value = false;
});

describe('Login', () => {
  it('stores entered credentials and clears needsLogin on submit', () => {
    render(<Login />);
    fireEvent.input(screen.getByLabelText('Password'),
      { target: { value: 'hunter2' } });
    fireEvent.click(screen.getByRole('button', { name: /sign in/i }));
    expect(getCreds()).toEqual({ user: 'admin', pass: 'hunter2' });
    expect(needsLogin.value).toBe(false);
  });

  it('shows an error when a prior login was rejected', () => {
    loginError.value = true;
    render(<Login />);
    expect(screen.getByText(/wrong username or password/i)).toBeInTheDocument();
  });
});
