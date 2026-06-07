import { ChannelBadge } from '../ChannelBadge/ChannelBadge';
import { NodeHeader } from '../NodeHeader/NodeHeader';
import type { NodeView } from '../../api/types';
import s from './NodeCard.module.css';

type Props = { node: NodeView; onAliasChange: (next: string) => void; };

export function NodeCard({ node, onAliasChange }: Props) {
  return (
    <div class={`${s.card} ${node.online ? s.online : s.offline}`}>
      <NodeHeader
        ieee={node.ieee}
        short_addr={node.short_addr}
        alias={node.alias}
        online={node.online}
        last_seen_s={node.last_seen_s}
        rssi_dbm={node.rssi_dbm}
        onAliasChange={onAliasChange}
      />
      <div class={s.badges}>
        {node.readings.map((r) => (
          <ChannelBadge key={`${r.kind}-${r.quantity}`}
                         kind={r.kind} quantity={r.quantity}
                         value={r.value} unit={r.unit} age_s={r.age_s} />
        ))}
      </div>
    </div>
  );
}
