import s from './ChannelBadge.module.css';
import type { Kind, Quantity } from '../../api/types';
import { formatReading } from '../../utils/format';
import { freshnessFor, formatAge } from '../../utils/freshness';

type Props = {
  kind: Kind;
  quantity: Quantity;
  value: number;
  unit: string;
  age_s: number;
};

export function ChannelBadge({ kind, quantity, value, age_s }: Props) {
  const f = freshnessFor(age_s);
  return (
    <div class={`${s.badge} ${s[f]}`} data-kind={kind}>
      <span class={s.value}>{formatReading(value, quantity)}</span>
      <span class={s.age}>{formatAge(age_s)}</span>
    </div>
  );
}
