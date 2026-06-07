import { describe, it, expect } from 'vitest';
import { uptime, ago, rssi } from '../format';

describe('uptime', () => {
  it('renders minutes only when <1h', () => expect(uptime(5 * 60)).toBe('5m'));
  it('renders h+m when <1d',         () => expect(uptime(3 * 3600 + 14 * 60)).toBe('3h 14m'));
  it('renders d+h when >=1d',        () => expect(uptime(2 * 86400 + 5 * 3600)).toBe('2d 5h'));
});

describe('ago', () => {
  it('returns "never" for null',  () => expect(ago(null)).toBe('never'));
  it('seconds for <1min',         () => {
    const NOW = 1_000_000;
    expect(ago(NOW - 15_000, NOW)).toBe('15s ago');
  });
  it('minutes for <1h',           () => {
    const NOW = 1_000_000;
    expect(ago(NOW - 5 * 60_000, NOW)).toBe('5m ago');
  });
});

describe('rssi', () => {
  it('strong @ -40', () => expect(rssi(-40)).toBe('strong'));
  it('good @ -60',   () => expect(rssi(-60)).toBe('good'));
  it('fair @ -70',   () => expect(rssi(-70)).toBe('fair'));
  it('weak @ -90',   () => expect(rssi(-90)).toBe('weak'));
});
