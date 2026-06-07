import { useState } from 'preact/hooks';
import s from './NodeHeader.module.css';
import { formatAge } from '../../utils/freshness';

type Props = {
  ieee: string;
  short_addr: string;
  alias: string | null;
  online: boolean;
  last_seen_s: number;
  rssi_dbm: number;
  onAliasChange: (next: string) => void;
};

export function NodeHeader({ short_addr, alias, online,
                              last_seen_s, rssi_dbm, onAliasChange }: Props) {
  const [editing, setEditing] = useState(false);
  const [draft, setDraft] = useState(alias ?? '');
  const display = alias ?? `Node ${short_addr}`;

  if (editing) {
    return (
      <div class={s.header}>
        <input
          class={s.input}
          value={draft}
          maxLength={23}
          onInput={(e) => setDraft((e.target as HTMLInputElement).value)}
          onKeyDown={(e) => {
            if (e.key === 'Enter') {
              setEditing(false);
              onAliasChange(draft.trim());
            }
            if (e.key === 'Escape') setEditing(false);
          }}
          onBlur={() => setEditing(false)}
          autoFocus
        />
      </div>
    );
  }

  return (
    <div class={s.header}>
      <span class={s.alias}>{display}</span>
      <button class={s.editBtn}
              aria-label="edit alias"
              onClick={() => { setDraft(alias ?? ''); setEditing(true); }}>
        ✏
      </button>
      <span class={`${s.pill} ${online ? s.online : s.offline}`}>
        {online ? 'online' : 'offline'}
      </span>
      <span class={s.meta}>{formatAge(last_seen_s)} · {rssi_dbm} dBm</span>
    </div>
  );
}
