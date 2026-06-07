import { Icon } from '../Icon';
import s from './Skeleton.module.css';

type Props = { label: string; iconId: string };

export function Skeleton({ label, iconId }: Props) {
  return (
    <div class={s.tile}>
      <div class={s.head}>
        <span class={s.icon}><Icon id={iconId} /></span>
        <span class={s.label}>{label}</span>
      </div>
      <div class={`${s.sk} ${s.num}`} />
      <div class={s.foot}><span class={`${s.sk} ${s.badge}`} /></div>
      <div class={`${s.sk} ${s.line}`} />
    </div>
  );
}
