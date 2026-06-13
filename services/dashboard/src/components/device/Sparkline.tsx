import { useId } from "react";

interface Props {
  points: number[];
  color?: string;
  /** Optional dashed reference line, in data space (e.g. soil trigger 30%). */
  markLine?: number;
  className?: string;
  height?: number;
}

const W = 240;
const PAD = 3;

/** Dependency-free SVG sparkline (area + line). Renders nothing for <2 points,
 *  so tiles degrade gracefully when no history is available. */
export function Sparkline({
  points,
  color = "hsl(var(--k-soil))",
  markLine,
  className,
  height = 36,
}: Props) {
  const id = useId();
  if (!points || points.length < 2) {
    return <div style={{ height }} className={className} aria-hidden />;
  }

  const H = height;
  let min = Math.min(...points);
  let max = Math.max(...points);
  if (markLine != null) {
    min = Math.min(min, markLine);
    max = Math.max(max, markLine);
  }
  const span = max - min || 1;
  const y = (v: number) => H - PAD - ((v - min) / span) * (H - PAD * 2);
  const x = (i: number) => (i / (points.length - 1)) * W;

  const line = points.map((v, i) => `${x(i).toFixed(1)},${y(v).toFixed(1)}`);
  const linePath = `M${line.join(" L")}`;
  const areaPath = `${linePath} L${W},${H} L0,${H} Z`;
  const gradId = `spark-${id}`;

  return (
    <svg
      viewBox={`0 0 ${W} ${H}`}
      preserveAspectRatio="none"
      className={className}
      style={{ width: "100%", height, display: "block", color }}
      role="img"
      aria-label="24-hour history sparkline"
    >
      <defs>
        <linearGradient id={gradId} x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stopColor="currentColor" stopOpacity="0.28" />
          <stop offset="100%" stopColor="currentColor" stopOpacity="0.02" />
        </linearGradient>
      </defs>
      <path d={areaPath} fill={`url(#${gradId})`} stroke="none" />
      <path
        d={linePath}
        fill="none"
        stroke="currentColor"
        strokeWidth={1.8}
        strokeLinecap="round"
        strokeLinejoin="round"
        vectorEffect="non-scaling-stroke"
      />
      {markLine != null && (
        <line
          x1="0"
          x2={W}
          y1={y(markLine)}
          y2={y(markLine)}
          stroke="hsl(var(--danger))"
          strokeWidth={1}
          strokeDasharray="3 3"
          opacity={0.6}
          vectorEffect="non-scaling-stroke"
        />
      )}
    </svg>
  );
}
