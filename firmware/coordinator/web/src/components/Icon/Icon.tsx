type Props = { id: string; size?: number; className?: string };

export function Icon({ id, size = 18, className }: Props) {
  return (
    <svg width={size} height={size} className={className} aria-hidden="true">
      <use href={`/icons.svg#${id}`} />
    </svg>
  );
}
