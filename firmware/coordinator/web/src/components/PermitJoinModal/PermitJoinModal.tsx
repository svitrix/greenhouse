import { useState, useEffect } from 'preact/hooks';
import s from './PermitJoinModal.module.css';

type Props = {
  open: boolean;
  onOpenWindow: (duration_s: number) => void;
  onClose: () => void;
};

export function PermitJoinModal({ open, onOpenWindow, onClose }: Props) {
  const [duration, setDuration] = useState(60);
  const [countdown, setCountdown] = useState(0);

  useEffect(() => {
    if (countdown <= 0) return;
    const t = setTimeout(() => setCountdown(countdown - 1), 1000);
    return () => clearTimeout(t);
  }, [countdown]);

  if (!open) return null;

  return (
    <div class={s.backdrop} onClick={onClose}>
      <div class={s.modal} onClick={(e) => e.stopPropagation()}>
        <h3>Add device</h3>
        <label>Window: {duration}s</label>
        <input type="range" min={1} max={254} value={duration}
               onInput={(e) =>
                 setDuration(parseInt((e.target as HTMLInputElement).value, 10))} />
        <div class={s.buttons}>
          <button onClick={() => { onOpenWindow(duration); setCountdown(duration); }}>
            Open network
          </button>
          <button onClick={onClose}>Cancel</button>
        </div>
        {countdown > 0 && (
          <div class={s.countdown}>Window closes in {countdown}s</div>
        )}
      </div>
    </div>
  );
}
