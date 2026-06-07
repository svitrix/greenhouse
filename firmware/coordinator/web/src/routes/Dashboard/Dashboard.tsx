import { useDashboard, getApi } from '../../state/useDashboard';
import { NodeCard } from '../../components/NodeCard/NodeCard';
import { PumpPanel } from '../../components/PumpPanel/PumpPanel';
import { AutoWaterPanel } from '../../components/AutoWaterPanel/AutoWaterPanel';
import { clearCreds, needsLogin } from '../../api/auth';
import s from './Dashboard.module.css';

export function Dashboard() {
  const { nodes, pump, auto, error } = useDashboard();
  const api = getApi();
  const signOut = () => { clearCreds(); needsLogin.value = true; };

  if (error) return <div class={s.error}>API error: {error}</div>;
  if (!nodes || !pump || !auto) return <div class={s.loading}>Loading…</div>;

  const onlineCount = nodes.nodes.filter((n) => n.online).length;

  return (
    <div class={s.dashboard}>
      <div class={s.header}>
        <span>Greenhouse · {nodes.nodes.length} nodes ({onlineCount} online)</span>
        <a href="#/pair" class={s.addBtn}>+ Add device</a>
        <button class={s.addBtn} type="button" onClick={signOut}>Sign out</button>
      </div>

      {nodes.nodes.length === 0 ? (
        <div class={s.empty}>No nodes paired yet. Click "+ Add device" to start.</div>
      ) : (
        nodes.nodes.map((n) => (
          <NodeCard key={n.ieee} node={n}
                     onAliasChange={(next) => void api.setAlias(n.ieee, next)} />
        ))
      )}

      <AutoWaterPanel state={auto} />
      <PumpPanel pump={pump}
                  onTogglePump={(st) => { void api.setPump(st); }} />
    </div>
  );
}
