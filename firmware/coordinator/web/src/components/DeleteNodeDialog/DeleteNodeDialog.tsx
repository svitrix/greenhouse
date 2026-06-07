import { useState } from 'preact/hooks';
import s from './DeleteNodeDialog.module.css';

type Props = {
  open: boolean;
  alias: string | null;
  short_addr: string;
  onConfirm: () => void;
  onCancel: () => void;
};

export function DeleteNodeDialog({ open, alias, short_addr,
                                    onConfirm, onCancel }: Props) {
  const [acked, setAcked]   = useState(false);
  const [draft, setDraft]   = useState('');
  if (!open) return null;

  const expected = alias ?? short_addr.slice(-4).toUpperCase();

  return (
    <div class={s.backdrop} onClick={onCancel}>
      <div class={s.modal} onClick={(e) => e.stopPropagation()}>
        {!acked ? (
          <>
            <h3>Forget {alias ?? `Node ${short_addr}`}?</h3>
            <p>
              This clears the alias, local history, and sends a Zigbee
              Mgmt_Leave_req. It does not delete the device — only
              unpairs it from this coordinator.
            </p>
            <div class={s.buttons}>
              <button onClick={() => setAcked(true)}>I understand</button>
              <button onClick={onCancel}>Cancel</button>
            </div>
          </>
        ) : (
          <>
            <p>Type <code>{expected}</code> to confirm:</p>
            <input type="text" role="textbox" value={draft}
                   onInput={(e) =>
                     setDraft((e.target as HTMLInputElement).value)} />
            <div class={s.buttons}>
              <button onClick={() => { if (draft === expected) onConfirm(); }}
                      disabled={draft !== expected}>
                Forget node
              </button>
              <button onClick={onCancel}>Cancel</button>
            </div>
          </>
        )}
      </div>
    </div>
  );
}
