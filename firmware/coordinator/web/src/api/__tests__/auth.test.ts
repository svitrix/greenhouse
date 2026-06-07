import { describe, it, expect, beforeEach } from 'vitest';
import {
  getCreds, setCreds, clearCreds, basicHeader, needsLogin,
} from '../auth';

beforeEach(() => {
  sessionStorage.clear();
  clearCreds();
  needsLogin.value = false;
});

describe('auth', () => {
  it('basicHeader returns Basic base64(user:pass)', () => {
    setCreds({ user: 'admin', pass: 'secret' });
    expect(basicHeader()).toBe('Basic ' + btoa('admin:secret'));
  });

  it('setCreds clears needsLogin', () => {
    needsLogin.value = true;
    setCreds({ user: 'admin', pass: 'x' });
    expect(needsLogin.value).toBe(false);
  });

  it('getCreds round-trips through sessionStorage', () => {
    setCreds({ user: 'admin', pass: 'x' });
    expect(getCreds()).toEqual({ user: 'admin', pass: 'x' });
  });

  it('clearCreds empties storage and basicHeader returns null', () => {
    setCreds({ user: 'a', pass: 'b' });
    clearCreds();
    expect(getCreds()).toBeNull();
    expect(basicHeader()).toBeNull();
  });

  it('encodes non-ASCII passwords without throwing', () => {
    setCreds({ user: 'admin', pass: 'pässwörd' });
    expect(() => basicHeader()).not.toThrow();
    expect(basicHeader())
      .toBe('Basic ' + btoa(unescape(encodeURIComponent('admin:pässwörd'))));
  });
});
