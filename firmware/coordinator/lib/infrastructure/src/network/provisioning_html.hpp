#pragma once
#include <pgmspace.h>

namespace gh::infra {

inline constexpr const char kProvisioningHtml[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Greenhouse Setup</title>
<style>
  *{box-sizing:border-box;font-family:-apple-system,sans-serif}
  body{max-width:500px;margin:0 auto;padding:20px;background:#f4f6f8;color:#222}
  h1{font-size:1.4rem}
  fieldset{border:1px solid #ccc;padding:14px;margin:14px 0;background:#fff;border-radius:6px}
  legend{padding:0 8px;font-weight:600}
  label{display:block;margin:8px 0 4px;font-size:.9rem}
  input,select{width:100%;padding:8px;border:1px solid #bbb;border-radius:4px;font-size:1rem}
  button{width:100%;padding:12px;background:#2a8;color:#fff;border:none;border-radius:4px;font-size:1rem;cursor:pointer;margin-top:10px}
  button:disabled{background:#aaa}
  #msg{margin-top:14px;padding:10px;border-radius:4px;display:none}
  .ok{background:#cfc;color:#040;display:block!important}
  .err{background:#fcc;color:#400;display:block!important}
  @media(prefers-color-scheme:dark){
    body{background:#222;color:#eee} fieldset{background:#333;border-color:#555}
    input,select{background:#444;color:#eee;border-color:#666}
  }
</style>
</head>
<body>
<h1>Greenhouse Setup Wizard</h1>
<form id="f">
  <fieldset>
    <legend>Wi-Fi</legend>
    <label>SSID</label>
    <select name="wifi_ssid" id="ssid_sel" required></select>
    <label>Password</label>
    <input name="wifi_password" type="password">
    <label>Hostname (optional)</label>
    <input name="wifi_hostname" placeholder="greenhouse">
  </fieldset>
  <fieldset>
    <legend>MQTT broker</legend>
    <label>Host</label><input name="mqtt_host" required>
    <label>Port</label><input name="mqtt_port" type="number" value="1883" required>
    <label>User</label><input name="mqtt_user">
    <label>Password</label><input name="mqtt_password" type="password">
    <label>Client ID</label><input name="mqtt_client_id" value="greenhouse">
    <label>Topic prefix</label><input name="mqtt_topic_prefix" value="greenhouse">
  </fieldset>
  <fieldset>
    <legend>Soil calibration</legend>
    <label>Raw value when dry (in air)</label>
    <input name="soil_dry" type="number" value="300" required>
    <label>Raw value when wet (in water)</label>
    <input name="soil_wet" type="number" value="700" required>
  </fieldset>
  <fieldset>
    <legend>Admin (web UI access)</legend>
    <label>Username</label>
    <input name="admin_user" placeholder="admin" maxlength="31" required>
    <label>Password (min 8 chars)</label>
    <input name="admin_password" type="password" minlength="8" maxlength="63" required>
    <label>Confirm password</label>
    <input name="admin_password_confirm" type="password" minlength="8" maxlength="63" required>
  </fieldset>
  <fieldset>
    <legend>Analytics backend (optional)</legend>
    <label>Backend URL <small>(leave empty to disable)</small></label>
    <input name="analytics_url" type="url" placeholder="http://192.168.1.42:8000/ingest" maxlength="127">
    <label>Pairing code <small>(6 digits from the admin UI)</small></label>
    <input name="analytics_code" type="text" pattern="\d{6}" maxlength="6" inputmode="numeric">
  </fieldset>
  <button type="submit" id="btn">Save &amp; Restart</button>
  <div id="msg"></div>
</form>
<script>
async function loadScan(){
  for (let attempts = 0; attempts < 12; attempts++) {
    try {
      const r = await fetch('/scan');
      const list = await r.json();
      if (r.status === 202) {
        await new Promise(res => setTimeout(res, list.retry_after_ms || 500));
        continue;
      }
      const sel = document.getElementById('ssid_sel');
      sel.innerHTML = list.networks.map(n =>
        `<option>${n.ssid}</option>`).join('');
      return;
    } catch (e) { break; }
  }
  const sel = document.getElementById('ssid_sel');
  sel.outerHTML = '<input name="wifi_ssid" required placeholder="Wi-Fi SSID">';
}
loadScan();
document.getElementById('f').addEventListener('submit', async e => {
  e.preventDefault();
  const btn = document.getElementById('btn');
  const msg = document.getElementById('msg');
  btn.disabled = true; btn.textContent = 'Saving...';
  msg.className = ''; msg.textContent = '';
  try {
    const r = await fetch('/save', {method: 'POST', body: new FormData(e.target)});
    const j = await r.json();
    if (r.ok) {
      msg.className = 'ok';
      msg.textContent = 'Saved. Device will restart in ~3 seconds.';
    } else {
      msg.className = 'err';
      msg.textContent = 'Error: ' + (j.error || 'unknown');
      btn.disabled = false; btn.textContent = 'Save & Restart';
    }
  } catch (err) {
    msg.className = 'err';
    msg.textContent = 'Network error: ' + err;
    btn.disabled = false; btn.textContent = 'Save & Restart';
  }
});
</script>
</body>
</html>
)HTML";

inline constexpr const char kProvisioningSuccessHtml[] PROGMEM =
    "<!DOCTYPE html><html><head><title>Greenhouse setup complete</title>"
    "<style>body{font-family:sans-serif;max-width:480px;margin:2em auto;padding:0 1em}"
    "ul{background:#f3f3f3;padding:1em 1.5em;border-radius:.4em}</style>"
    "</head><body>"
    "<h1>&#9989; Setup complete</h1>"
    "<p>The coordinator will reboot and connect to your Wi-Fi.</p>"
    "<p>After reboot, open <code>http://%s/</code> in a browser.</p>"
    "<p>The browser will ask for credentials:</p>"
    "<ul>"
      "<li>Username: <code>%s</code></li>"
      "<li>Password: <em>(the one you just set)</em></li>"
    "</ul>"
    "</body></html>";

}
