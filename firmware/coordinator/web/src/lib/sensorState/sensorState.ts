export type BadgeKind =
  | 'is-cold' | 'is-cool' | 'is-optimal' | 'is-warm' | 'is-hot'
  | 'is-dry' | 'is-moist' | 'is-wet' | 'is-humid' | 'is-saturated'
  | 'is-alert' | 'is-offline';

export type StateBadge = readonly [label: string, badgeClass: BadgeKind];

export function airTempState(c: number): StateBadge {
  if (c < 10) return ['cold', 'is-cold'];
  if (c < 18) return ['cool', 'is-cool'];
  if (c <= 28) return ['optimal', 'is-optimal'];
  if (c <= 32) return ['warm', 'is-warm'];
  return ['hot', 'is-hot'];
}

export function airHumidityState(pct: number): StateBadge {
  if (pct < 40)  return ['dry',       'is-dry'];
  if (pct <= 75) return ['optimal',   'is-optimal'];
  if (pct <= 90) return ['humid',     'is-humid'];
  return ['saturated', 'is-saturated'];
}

export function soilMoistureState(pct: number): StateBadge {
  if (pct < 30)  return ['dry · water me', 'is-dry'];
  if (pct <= 70) return ['moist',          'is-moist'];
  if (pct <= 90) return ['wet',            'is-wet'];
  return ['saturated', 'is-saturated'];
}

export function soilTempState(c: number): StateBadge {
  if (c < 10)  return ['cold',    'is-cold'];
  if (c <= 25) return ['optimal', 'is-optimal'];
  if (c <= 30) return ['warm',    'is-warm'];
  return ['hot', 'is-hot'];
}
