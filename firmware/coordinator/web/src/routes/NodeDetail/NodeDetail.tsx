import { useEffect, useState } from 'preact/hooks';
import { NodeHeader } from '../../components/NodeHeader/NodeHeader';
import { HistoryChart } from '../../components/HistoryChart/HistoryChart';
import { DeleteNodeDialog } from '../../components/DeleteNodeDialog/DeleteNodeDialog';
import { Api } from '../../api/client';
import type { NodeView } from '../../api/types';
import { formatReading } from '../../utils/format';
import s from './NodeDetail.module.css';

const api = new Api();

type Props = { ieee: string };

export function NodeDetail({ ieee }: Props) {
  const [node, setNode]       = useState<NodeView | null>(null);
  const [hours, setHours]     = useState<1 | 6 | 24>(6);
  const [history, setHistory] = useState<Record<string, { ts_ms: number; value: number }[]>>({});
  const [delDlg, setDelDlg]   = useState(false);

  useEffect(() => {
    let alive = true;
    const tick = async () => {
      try {
        const n = await api.getNode(ieee);
        if (!alive) return;
        setNode(n);
        for (const r of n.readings) {
          const key = `${r.kind}-${r.quantity}`;
          const h = await api.history(ieee, r.kind, r.quantity, hours);
          if (alive) setHistory((prev) => ({ ...prev, [key]: h.data }));
        }
      } catch {
        // keep last state on transient errors
      }
    };
    void tick();
    const t = setInterval(() => { void tick(); }, 5000);
    return () => { alive = false; clearInterval(t); };
  }, [ieee, hours]);

  if (!node) return <div class={s.loading}>Loading…</div>;

  return (
    <div class={s.detail}>
      <a href="#/" class={s.back}>← Back</a>
      <NodeHeader ieee={node.ieee} short_addr={node.short_addr}
                  alias={node.alias} online={node.online}
                  last_seen_s={node.last_seen_s} rssi_dbm={node.rssi_dbm}
                  onAliasChange={(next) => void api.setAlias(ieee, next)} />
      <div class={s.tabs}>
        {([1, 6, 24] as const).map((h) => (
          <button key={h}
                  onClick={() => setHours(h)}
                  class={h === hours ? s.tabActive : s.tab}
                  disabled={h === hours}>{h}h</button>
        ))}
      </div>
      {node.readings.map((r) => {
        const key = `${r.kind}-${r.quantity}`;
        return (
          <div key={key} class={s.chartBlock}>
            <div class={s.chartHead}>
              <span>{r.kind} · {r.quantity}</span>
              <strong>{formatReading(r.value, r.quantity)}</strong>
            </div>
            <HistoryChart points={history[key] ?? []} width={320} height={80} />
          </div>
        );
      })}
      <button class={s.forgetBtn} onClick={() => setDelDlg(true)}>
        Forget node
      </button>
      <DeleteNodeDialog
        open={delDlg}
        alias={node.alias}
        short_addr={node.short_addr}
        onConfirm={async () => {
          await api.deleteNode(ieee);
          location.hash = '#/';
        }}
        onCancel={() => setDelDlg(false)} />
    </div>
  );
}
