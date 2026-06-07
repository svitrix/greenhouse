import s from './HistoryChart.module.css';

type Point = { ts_ms: number; value: number; };
type Props  = { points: Point[]; width: number; height: number; };

export function HistoryChart({ points, width, height }: Props) {
  if (points.length === 0) {
    return <div class={s.empty}>no data</div>;
  }
  const xs = points.map((p) => p.ts_ms);
  const ys = points.map((p) => p.value);
  const xMin = Math.min(...xs);
  const xMax = Math.max(...xs);
  const yMin = Math.min(...ys);
  const yMax = Math.max(...ys);
  const pad = 4;
  const xSpan = (xMax - xMin) || 1;
  const ySpan = (yMax - yMin) || 1;
  const project = (p: Point) => {
    const x = pad + ((p.ts_ms - xMin) / xSpan) * (width  - 2 * pad);
    const y = pad + (1 - (p.value  - yMin) / ySpan) * (height - 2 * pad);
    return [x, y] as const;
  };
  const d = points.map((p, i) => {
    const [x, y] = project(p);
    return `${i === 0 ? 'M' : 'L'}${x.toFixed(2)},${y.toFixed(2)}`;
  }).join(' ');

  return (
    <svg class={s.svg} width={width} height={height}
         viewBox={`0 0 ${width} ${height}`}>
      <path d={d} fill="none" stroke="currentColor" strokeWidth={1.5} />
    </svg>
  );
}
