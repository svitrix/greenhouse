import { describe, it, expect } from 'vitest';
import { airTempState, airHumidityState, soilMoistureState, soilTempState } from '../sensorState';

describe('airTempState', () => {
  it('cold',    () => expect(airTempState(5)[0]).toBe('cold'));
  it('cool',    () => expect(airTempState(15)[0]).toBe('cool'));
  it('optimal', () => expect(airTempState(22)[0]).toBe('optimal'));
  it('warm',    () => expect(airTempState(30)[0]).toBe('warm'));
  it('hot',     () => expect(airTempState(40)[0]).toBe('hot'));
});

describe('airHumidityState', () => {
  it('dry',       () => expect(airHumidityState(30)[0]).toBe('dry'));
  it('optimal',   () => expect(airHumidityState(60)[0]).toBe('optimal'));
  it('humid',     () => expect(airHumidityState(85)[0]).toBe('humid'));
  it('saturated', () => expect(airHumidityState(95)[0]).toBe('saturated'));
});

describe('soilMoistureState', () => {
  it('dry',       () => expect(soilMoistureState(20)[0]).toMatch(/dry/));
  it('moist',     () => expect(soilMoistureState(50)[0]).toBe('moist'));
  it('wet',       () => expect(soilMoistureState(80)[0]).toBe('wet'));
  it('saturated', () => expect(soilMoistureState(95)[0]).toBe('saturated'));
});

describe('soilTempState', () => {
  it('cold',    () => expect(soilTempState(5)[0]).toBe('cold'));
  it('optimal', () => expect(soilTempState(20)[0]).toBe('optimal'));
  it('warm',    () => expect(soilTempState(28)[0]).toBe('warm'));
  it('hot',     () => expect(soilTempState(35)[0]).toBe('hot'));
});
