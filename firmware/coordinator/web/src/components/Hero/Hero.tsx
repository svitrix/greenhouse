import s from './Hero.module.css';

type Props = { greetingOverride?: string; updatedText?: string };

export function Hero({ greetingOverride, updatedText }: Props = {}) {
  const hour = new Date().getHours();
  const greet = greetingOverride ??
    (hour < 6  ? 'Quiet hours in the greenhouse.' :
     hour < 12 ? 'Good morning, gardener.'        :
     hour < 18 ? 'Afternoon report.'              :
                 'Evening wrap-up.');
  const updated = updatedText ?? 'updated just now';
  return (
    <div class={s.hero}>
      <span class={s.greeting}>{greet}</span>
      <span class={s.updated}><span class={s.pulse} /> {updated}</span>
    </div>
  );
}
