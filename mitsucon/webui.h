// Web interface pages, served from flash

const char PAGE_STYLE[] PROGMEM = R"rawliteral(
<style>
:root{--bg:#f4f5f7;--card:#fff;--fg:#1c1e21;--mute:#6b7280;--line:#e2e5e9;--acc:#2563eb;--hot:#ea580c;--cold:#0284c7}
@media (prefers-color-scheme:dark){:root{--bg:#111316;--card:#1b1e23;--fg:#e8eaed;--mute:#9aa0a6;--line:#2c3036;--acc:#60a5fa;--hot:#fb923c;--cold:#38bdf8}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.4 system-ui,sans-serif}
main{max-width:560px;margin:0 auto;padding:16px}h1{font-size:20px;margin:4px 0 16px}h2{font-size:13px;text-transform:uppercase;letter-spacing:.05em;color:var(--mute);margin:0 0 10px}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:16px;margin-bottom:12px}
.big{display:flex;justify-content:space-between;align-items:flex-end}.big b{font-size:44px;font-weight:600;line-height:1}.big small{color:var(--mute)}
.row{display:flex;gap:8px;flex-wrap:wrap}.row>*{flex:1}
button,select,input{font:inherit;color:inherit;background:var(--bg);border:1px solid var(--line);border-radius:8px;padding:9px 10px;min-width:0}
button{cursor:pointer}button.on{background:var(--acc);border-color:var(--acc);color:#fff}button:disabled{opacity:.5}
.set{display:flex;align-items:center;gap:12px}.set b{font-size:32px;flex:1;text-align:center}.set button{flex:0 0 56px;font-size:22px}
dl{display:grid;grid-template-columns:auto 1fr;gap:4px 16px;margin:0}dt{color:var(--mute)}dd{margin:0;text-align:right;overflow-wrap:anywhere}
label{display:block;margin:10px 0 4px;color:var(--mute)}input{width:100%}a{color:var(--acc)}
.top{display:flex;justify-content:space-between;align-items:center;gap:12px;margin:4px 0 16px}.top h1{margin:0}
.seg{display:flex;flex:0 0 auto}.seg button{padding:5px 10px;border-radius:0}.seg button:first-child{border-radius:8px 0 0 8px}.seg button:last-child{border-radius:0 8px 8px 0;border-left:0}
.heat{color:var(--hot)}.cool{color:var(--cold)}#msg{min-height:1.4em;color:var(--mute)}
</style>
)rawliteral";

const char PAGE_INDEX[] PROGMEM = R"rawliteral(
<main>
<div class="top"><h1 id="name">Heat Pump</h1>
 <div class="seg" id="units"><button data-u="C" onclick="setUnit('C')">&deg;C</button><button data-u="F" onclick="setUnit('F')">&deg;F</button></div></div>
<div class="card"><h2>Room</h2>
 <div class="big"><b id="room">--</b><small id="action">--</small></div></div>
<div class="card"><h2>Target</h2>
 <div class="set"><button onclick="step(-1)">&minus;</button><b id="target">--</b><button onclick="step(1)">+</button></div></div>
<div class="card"><h2>Mode</h2>
 <div class="row" id="modes"></div></div>
<div class="card"><h2>Fan &amp; vane</h2>
 <div class="row"><select id="fan" onchange="send('fan',this.value)"></select><select id="vane" onchange="send('vane',this.value)"></select></div></div>
<div class="card"><h2>Controller</h2><dl id="stats"></dl></div>
<p id="msg"></p>
<p><a href="/config">Settings</a> &middot; <a href="/update">Firmware update</a></p>
</main>
<script>
const MODES=[['off','Off'],['heat','Heat'],['cool','Cool'],['heat_cool','Auto'],['dry','Dry'],['fan_only','Fan']];
const FANS=['auto','quiet','1','2','3','4'],VANES=['auto','1','2','3','4','5','swing'];
let st=null,pending=null,timer=null;
const $=id=>document.getElementById(id);
const esc=t=>String(t).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'})[c]);
// display unit only, the controller and Home Assistant always exchange Celsius
let unit='C';try{unit=localStorage.getItem('unit')||'C'}catch(e){}
const toF=c=>c*9/5+32;
const fmt=c=>unit=='F'?Math.round(toF(c))+'\u00b0F':c.toFixed(1)+'\u00b0C';
function setUnit(u){
 unit=u;try{localStorage.setItem('unit',u)}catch(e){}
 document.querySelectorAll('#units button').forEach(b=>b.classList.toggle('on',b.dataset.u==u));
 if(st)render(st);if(pending!==null)$('target').textContent=fmt(pending);
}
setUnit(unit);
$('modes').innerHTML=MODES.map(m=>`<button data-m="${m[0]}" onclick="send('mode','${m[0]}')">${m[1]}</button>`).join('');
$('fan').innerHTML=FANS.map(f=>`<option value="${f}">Fan ${f}</option>`).join('');
$('vane').innerHTML=VANES.map(v=>`<option value="${v}">Vane ${v}</option>`).join('');
function up(t){
 const d=Math.floor(t/86400),h=Math.floor(t/3600)%24,m=Math.floor(t/60)%60;
 return (d?d+'d ':'')+h+'h '+m+'m';
}
function render(s){
 st=s;const hp=s.hp;
 document.title=s.name;$('name').textContent=s.name;
 $('room').textContent=hp.connected&&hp.roomTemperature?fmt(hp.roomTemperature):'--';
 const a=hp.action;$('action').textContent=a;$('action').className=a=='heating'?'heat':a=='cooling'?'cool':'';
 if(pending===null)$('target').textContent=fmt(hp.temperature);
 document.querySelectorAll('#modes button').forEach(b=>b.classList.toggle('on',b.dataset.m==hp.mode));
 if(document.activeElement!==$('fan'))$('fan').value=hp.fan;
 if(document.activeElement!==$('vane'))$('vane').value=hp.vane;
 const rows=[['Heat pump link',hp.connected?'connected':'not connected'],['Compressor',hp.operating?'running':'idle'],
  ['MQTT',!s.mqtt.server?'not configured':s.mqtt.connected?'connected ('+s.mqtt.server+')':'disconnected'],
  ['WiFi',!s.wifi.ssid?'not configured':s.wifi.connected?s.wifi.ssid+' ('+s.wifi.rssi+' dBm)':'not connected'],['IP',s.wifi.ip],
  ['Fallback AP',s.wifi.ap?s.wifi.ap_ssid+' ('+s.wifi.ap_clients+' clients)':'off'],
  ['Uptime',up(s.uptime)],['Free memory',s.heap+' bytes'],['Last reset',s.reset],['Firmware',s.fw]];
 $('stats').innerHTML=rows.map(r=>`<dt>${r[0]}</dt><dd>${esc(r[1])}</dd>`).join('');
}
function step(dir){
 if(!st)return;const l=st.limits;
 const cur=pending===null?st.hp.temperature:pending;let t=cur+dir*l.step;
 if(unit=='F'){
  // step 1 degree F, send the nearest Celsius value the unit accepts
  let f=Math.round(toF(cur));t=cur;
  for(let i=0;i<6&&t==cur;i++){f+=dir;t=Math.round((f-32)*5/9/l.step)*l.step;}
 }
 t=Math.min(l.max,Math.max(l.min,t));
 pending=t;$('target').textContent=fmt(t);
 clearTimeout(timer);timer=setTimeout(()=>{send('temp',pending);pending=null},800);
}
async function send(k,v){
 $('msg').textContent='Sending…';
 try{const r=await fetch('/api/set',{method:'POST',body:new URLSearchParams({[k]:v})});
  if(!r.ok)throw new Error(await r.text());$('msg').textContent='';
  if(k=='temp')st.hp.temperature=+v;else st.hp[k]=v;render(st);setTimeout(poll,2000);}
 catch(e){$('msg').textContent='Failed: '+e.message}
}
async function poll(){
 try{const r=await fetch('/api/status');render(await r.json())}catch(e){$('msg').textContent='Controller unreachable'}
}
poll();setInterval(poll,5000);
</script>
)rawliteral";

const char PAGE_CONFIG[] PROGMEM = R"rawliteral(
<main>
<h1>Settings</h1>
<form class="card" id="f" onsubmit="save(event)"><h2>WiFi</h2>
 <label for="ssid">Network</label><input id="ssid" name="ssid" list="nets" required maxlength="32">
 <datalist id="nets"></datalist>
 <div class="row" style="margin-top:8px"><button type="button" onclick="scan()" id="sb">Scan networks</button></div>
 <label for="pass">Password (leave empty to keep current)</label><input id="pass" name="pass" type="password" maxlength="64">
 <h2 style="margin-top:20px">MQTT</h2>
 <label for="mqtt_server">Server</label><input id="mqtt_server" name="mqtt_server" required maxlength="64">
 <label for="mqtt_port">Port</label><input id="mqtt_port" name="mqtt_port" type="number" min="1" max="65535" required>
 <label for="mqtt_user">User</label><input id="mqtt_user" name="mqtt_user" maxlength="32">
 <label for="mqtt_pass">Password (leave empty to keep current)</label><input id="mqtt_pass" name="mqtt_pass" type="password" maxlength="64">
 <div class="row" style="margin-top:16px"><button class="on" type="submit">Save &amp; reboot</button></div>
</form>
<div class="card"><h2>Controller</h2><div class="row">
 <button onclick="act('/api/reboot','Reboot the controller?')">Reboot</button>
 <button onclick="act('/api/factory','Forget saved settings and reboot with firmware defaults?')">Reset settings</button></div></div>
<p id="msg"></p>
<p><a href="/">&larr; Back</a></p>
</main>
<script>
const $=id=>document.getElementById(id);
const esc=t=>String(t).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'})[c]);
fetch('/api/config').then(r=>r.json()).then(c=>{for(const k of ['ssid','mqtt_server','mqtt_port','mqtt_user'])$(k).value=c[k];
 if(c.stored===false)$('msg').textContent='Using firmware defaults';});
async function scan(){
 $('sb').disabled=true;$('sb').textContent='Scanning…';
 try{const n=await (await fetch('/api/scan')).json();
  n.sort((a,b)=>b.rssi-a.rssi);
  $('nets').innerHTML=n.map(x=>`<option value="${esc(x.ssid)}">${x.rssi} dBm${x.open?' open':''}</option>`).join('');
  $('sb').textContent=n.length+' networks found';}
 catch(e){$('sb').textContent='Scan failed'}
 $('sb').disabled=false;
}
async function save(e){
 e.preventDefault();$('msg').textContent='Saving…';
 const r=await fetch('/api/config',{method:'POST',body:new URLSearchParams(new FormData($('f')))});
 $('msg').textContent=r.ok?'Saved, rebooting. Reconnect to your normal network.':'Failed: '+await r.text();
}
async function act(u,q){
 if(!confirm(q))return;const r=await fetch(u,{method:'POST'});$('msg').textContent=await r.text();
}
</script>
)rawliteral";
