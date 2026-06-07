import { useEffect, useState } from 'preact/hooks';
import { PermitJoinModal } from '../../components/PermitJoinModal/PermitJoinModal';
import { Api } from '../../api/client';

const api = new Api();

export function Pair() {
  const [seen, setSeen] = useState<Set<string>>(new Set());
  const [joined, setJoined] = useState<string[]>([]);
  useEffect(() => {
    let alive = true;
    void (async () => {
      try {
        const r = await api.listNodes();
        if (alive) setSeen(new Set(r.nodes.map((n) => n.ieee)));
      } catch { /* ignore */ }
    })();
    const t = setInterval(async () => {
      try {
        const r = await api.listNodes();
        const newOnes = r.nodes.map((n) => n.ieee).filter((id) => !seen.has(id));
        if (newOnes.length) setJoined((prev) => [...prev, ...newOnes]);
      } catch { /* ignore */ }
    }, 2000);
    return () => { alive = false; clearInterval(t); };
  }, [seen]);

  return (
    <>
      <PermitJoinModal open={true}
                       onOpenWindow={(d) => { void api.permitJoin(d); }}
                       onClose={() => { location.hash = '#/'; }} />
      {joined.length > 0 && (
        <ul>{joined.map((id) => <li key={id}>✨ {id} joined</li>)}</ul>
      )}
    </>
  );
}
