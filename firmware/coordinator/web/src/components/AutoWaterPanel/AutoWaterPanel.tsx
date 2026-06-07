import type { AutoWaterState } from '../../api/types';
import s from './AutoWaterPanel.module.css';

type Props = { state: AutoWaterState };

export function AutoWaterPanel({ state }: Props) {
  const avg = state.avg_moisture_pct == null
    ? '—' : `${state.avg_moisture_pct.toFixed(0)}%`;
  return (
    <div class={s.panel}>
      <div class={s.title}>Auto-water</div>
      <div>avg {avg} · {state.fresh_sources.length} fresh,
        {' '}{state.stale_sources.length} stale</div>
      <div class={s.decision}>{state.last_decision}</div>
    </div>
  );
}
