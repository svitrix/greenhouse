import type { PumpView } from '../../api/types';
import s from './PumpPanel.module.css';

type Props = { pump: PumpView; onTogglePump: (state: 'ON' | 'OFF') => void; };

export function PumpPanel({ pump, onTogglePump }: Props) {
  return (
    <div class={s.panel}>
      <div class={s.row}>
        <span class={s.label}>Pump</span>
        <span class={`${s.pill} ${s[pump.state.toLowerCase()]}`}>{pump.state}</span>
      </div>
      {pump.lockout_reason && (
        <div class={s.lockout}>locked: {pump.lockout_reason}</div>
      )}
      <div class={s.buttons}>
        <button onClick={() => onTogglePump('ON')}
                disabled={pump.state !== 'OFF'}>Run 10s</button>
        <button onClick={() => onTogglePump('OFF')}
                disabled={pump.state === 'OFF'}>Stop</button>
      </div>
    </div>
  );
}
