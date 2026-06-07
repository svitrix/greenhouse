import { describe, expect, it } from 'vitest';
import { freshnessFor, formatAge } from '../freshness';
import { formatReading } from '../format';
import { formatIeeeColons } from '../ieee';

describe('freshnessFor', () => {
  it('returns fresh/recent/stale', () => {
    expect(freshnessFor(5)).toBe('fresh');
    expect(freshnessFor(60)).toBe('recent');
    expect(freshnessFor(600)).toBe('stale');
  });
});

describe('formatAge', () => {
  it('formats seconds compactly', () => {
    expect(formatAge(5)).toBe('5s');
    expect(formatAge(75)).toBe('1m 15s');
    expect(formatAge(3700)).toBe('1h 1m');
  });
});

describe('formatReading', () => {
  it('formats by quantity', () => {
    expect(formatReading(23.4, 'temp_c')).toBe('23.4 °C');
    expect(formatReading(42, 'moisture_pct')).toBe('42 %');
    expect(formatReading(4.05, 'voltage_v')).toBe('4.05 V');
  });
});

describe('formatIeeeColons', () => {
  it('inserts colons every 2 chars', () => {
    expect(formatIeeeColons('00124B001A2B3C4D')).toBe('00:12:4B:00:1A:2B:3C:4D');
  });
});
