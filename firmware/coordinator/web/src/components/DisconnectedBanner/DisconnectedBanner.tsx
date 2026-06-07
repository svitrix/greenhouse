import { Icon } from '../Icon';
import s from './DisconnectedBanner.module.css';

type Props = { onReload: () => void };

export function DisconnectedBanner({ onReload }: Props) {
  return (
    <div class={s.banner}>
      <span class={s.art}><Icon id="i-wifi-off" size={36} /></span>
      <div class={s.title}>Can't reach the coordinator.</div>
      <div class={s.body}>
        Your phone is on the network, but the greenhouse box isn't responding.
        It may be rebooting, or it dropped off Wi-Fi. Try again in a moment.
      </div>
      <button class={s.cta} onClick={onReload}>
        <Icon id="i-refresh" size={16} /> Try again
      </button>
    </div>
  );
}
