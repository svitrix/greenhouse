import { useState } from 'preact/hooks';
import { setCreds, loginError } from '../../api/auth';
import s from './Login.module.css';

// Shown when the device is operational but no valid credentials are stored
// (initial load or after a 401). Storing credentials flips `needsLogin` and
// re-renders App into the dashboard; wrong credentials produce a fresh 401
// (which sets `loginError`) and route straight back here with a message.
export function Login() {
  const [user, setUser] = useState('admin');
  const [pass, setPass] = useState('');

  const onSubmit = (e: Event) => {
    e.preventDefault();
    setCreds({ user, pass });
  };

  return (
    <div class={s.login}>
      <form class={s.card} onSubmit={onSubmit}>
        <h1 class={s.title}>Greenhouse</h1>
        <p class={s.sub}>Sign in to the admin dashboard.</p>
        {loginError.value &&
          <div class={s.err} role="alert">Wrong username or password.</div>}
        <label class={s.field}>
          <span>Username</span>
          <input class={s.input} type="text" name="username" autocomplete="username"
                 value={user}
                 onInput={(e) => setUser((e.target as HTMLInputElement).value)} />
        </label>
        <label class={s.field}>
          <span>Password</span>
          <input class={s.input} type="password" name="password"
                 autocomplete="current-password" required
                 value={pass}
                 onInput={(e) => setPass((e.target as HTMLInputElement).value)} />
        </label>
        <button class={s.btn} type="submit">Sign in</button>
      </form>
    </div>
  );
}
