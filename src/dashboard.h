#pragma once
#include <string_view>

static constexpr std::string_view DASHBOARD_HTML = R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>HERMES Dashboard</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4/dist/chart.umd.min.js"></script>
<style>
*{margin:0;padding:0;box-sizing:border-box}
:root{--bg:#0f172a;--card:#1e293b;--border:#334155;--text:#e2e8f0;--muted:#94a3b8;--accent:#3b82f6;--green:#22c55e;--red:#ef4444;--yellow:#eab308}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:var(--bg);color:var(--text);min-height:100vh}
nav{background:var(--card);border-bottom:1px solid var(--border);padding:12px 24px;display:flex;align-items:center;justify-content:space-between}
nav h1{font-size:1.25rem;font-weight:700;letter-spacing:1px}
nav h1 span{color:var(--accent)}
.status{display:flex;align-items:center;gap:8px;font-size:.85rem;color:var(--muted)}
.dot{width:8px;height:8px;border-radius:50%;background:var(--green)}
.dot.off{background:var(--red)}
#auth-modal{position:fixed;inset:0;background:rgba(0,0,0,.7);display:flex;align-items:center;justify-content:center;z-index:100}
#auth-modal .box{background:var(--card);border:1px solid var(--border);border-radius:12px;padding:32px;width:360px;text-align:center}
#auth-modal input{width:100%;padding:10px 14px;margin:16px 0;border:1px solid var(--border);border-radius:8px;background:var(--bg);color:var(--text);font-size:.95rem}
#auth-modal button{background:var(--accent);color:#fff;border:none;padding:10px 24px;border-radius:8px;cursor:pointer;font-size:.95rem}
main{max-width:1200px;margin:0 auto;padding:24px}
.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:16px;margin-bottom:24px}
.card{background:var(--card);border:1px solid var(--border);border-radius:12px;padding:20px}
.card .label{font-size:.8rem;color:var(--muted);text-transform:uppercase;letter-spacing:.5px}
.card .value{font-size:1.75rem;font-weight:700;margin-top:4px}
.card .sub{font-size:.8rem;color:var(--muted);margin-top:2px}
.charts{display:grid;grid-template-columns:1fr 1fr;gap:16px;margin-bottom:24px}
@media(max-width:768px){.charts{grid-template-columns:1fr}}
.chart-box{background:var(--card);border:1px solid var(--border);border-radius:12px;padding:16px}
.chart-box h3{font-size:.9rem;margin-bottom:12px;color:var(--muted)}
section{margin-bottom:24px}
section h2{font-size:1.1rem;margin-bottom:12px;color:var(--muted)}
table{width:100%;border-collapse:collapse;background:var(--card);border-radius:12px;overflow:hidden;border:1px solid var(--border)}
th,td{padding:10px 14px;text-align:left;border-bottom:1px solid var(--border);font-size:.85rem}
th{background:var(--bg);color:var(--muted);font-weight:600;text-transform:uppercase;font-size:.75rem;letter-spacing:.5px}
.badge{display:inline-block;padding:2px 8px;border-radius:4px;font-size:.75rem;font-weight:600}
.badge.active{background:rgba(34,197,94,.15);color:var(--green)}
.badge.revoked{background:rgba(239,68,68,.15);color:var(--red)}
.btn{padding:6px 14px;border:none;border-radius:6px;cursor:pointer;font-size:.8rem;font-weight:600}
.btn-primary{background:var(--accent);color:#fff}
.btn-danger{background:var(--red);color:#fff}
.btn-sm{padding:4px 10px;font-size:.75rem}
.actions{display:flex;gap:8px;margin-bottom:12px}
#create-key-form{display:none;background:var(--card);border:1px solid var(--border);border-radius:12px;padding:16px;margin-bottom:12px}
#create-key-form input{padding:8px 12px;border:1px solid var(--border);border-radius:6px;background:var(--bg);color:var(--text);font-size:.85rem;margin-right:8px}
.tab-bar{display:flex;gap:4px;margin-bottom:16px}
.tab{padding:8px 16px;border-radius:8px 8px 0 0;cursor:pointer;font-size:.85rem;color:var(--muted);background:transparent;border:1px solid transparent}
.tab.active{background:var(--card);color:var(--text);border-color:var(--border);border-bottom-color:var(--card)}
.tab-content{display:none}.tab-content.active{display:block}
</style>
</head>
<body>
<nav>
  <h1><span>HERMES</span> Dashboard</h1>
  <div class="status"><div class="dot" id="health-dot"></div><span id="health-text">Checking...</span></div>
</nav>

<div id="auth-modal">
  <div class="box">
    <h2 style="margin-bottom:8px">Admin Authentication</h2>
    <p style="color:var(--muted);font-size:.85rem">Enter your admin API key to access the dashboard</p>
    <input type="password" id="key-input" placeholder="sk-... or admin key" autofocus>
    <button onclick="authenticate()">Connect</button>
    <p id="auth-error" style="color:var(--red);font-size:.8rem;margin-top:8px"></p>
  </div>
</div>

<main>
  <div class="cards">
    <div class="card"><div class="label">Uptime</div><div class="value" id="c-uptime">--</div></div>
    <div class="card"><div class="label">Total Requests</div><div class="value" id="c-requests">--</div></div>
    <div class="card"><div class="label">Active Now</div><div class="value" id="c-active">--</div></div>
    <div class="card"><div class="label">Memory</div><div class="value" id="c-memory">--</div><div class="sub" id="c-memory-sub"></div></div>
    <div class="card"><div class="label">Cache Hit Rate</div><div class="value" id="c-cache">--</div><div class="sub" id="c-cache-sub"></div></div>
  </div>

  <div class="charts">
    <div class="chart-box"><h3>Requests / min</h3><canvas id="chart-rpm"></canvas></div>
    <div class="chart-box"><h3>Latency (ms)</h3><canvas id="chart-latency"></canvas></div>
  </div>

  <div class="tab-bar">
    <div class="tab active" onclick="switchTab('keys')">API Keys</div>
    <div class="tab" onclick="switchTab('usage')">Usage</div>
    <div class="tab" onclick="switchTab('audit')">Activity</div>
    <div class="tab" onclick="switchTab('plugins')">Plugins</div>
  </div>

  <div id="tab-keys" class="tab-content active">
    <div class="actions">
      <button class="btn btn-primary" onclick="toggleCreateKey()">+ Create Key</button>
    </div>
    <div id="create-key-form">
      <input id="new-alias" placeholder="Alias (e.g. dev-team)">
      <input id="new-rpm" placeholder="Rate limit RPM (0=unlimited)" type="number" value="0" style="width:180px">
      <button class="btn btn-primary" onclick="createKey()">Create</button>
      <button class="btn" onclick="toggleCreateKey()" style="background:var(--border)">Cancel</button>
      <p id="new-key-result" style="margin-top:8px;font-size:.85rem;color:var(--green)"></p>
    </div>
    <table><thead><tr><th>Alias</th><th>Prefix</th><th>Created</th><th>Last Used</th><th>Requests</th><th>RPM</th><th>Status</th><th></th></tr></thead>
    <tbody id="keys-body"></tbody></table>
  </div>

  <div id="tab-usage" class="tab-content">
    <table><thead><tr><th>Key</th><th>Daily Tokens</th><th>Daily Requests</th><th>Monthly Tokens</th><th>Monthly Requests</th></tr></thead>
    <tbody id="usage-body"></tbody></table>
  </div>

  <div id="tab-audit" class="tab-content">
    <table><thead><tr><th>Time</th><th>Key</th><th>Method</th><th>Model</th><th>Status</th><th>Tokens</th><th>Latency</th></tr></thead>
    <tbody id="audit-body"></tbody></table>
  </div>

  <div id="tab-plugins" class="tab-content">
    <table><thead><tr><th>Name</th><th>Version</th><th>Type</th><th>Position</th></tr></thead>
    <tbody id="plugins-body"></tbody></table>
  </div>
</main>

<script>
let API_KEY='';const H=()=>({Authorization:'Bearer '+API_KEY});
const rpmData=[];const latData=[];let rpmChart,latChart;

function authenticate(){
  API_KEY=document.getElementById('key-input').value;
  if(!API_KEY){document.getElementById('auth-error').textContent='Key is required';return}
  sessionStorage.setItem('hermes_key',API_KEY);
  fetch('/health').then(r=>r.json()).then(()=>{
    document.getElementById('auth-modal').style.display='none';
    startPolling();
  }).catch(()=>{document.getElementById('auth-error').textContent='Connection failed'});
}

function switchTab(name){
  document.querySelectorAll('.tab').forEach((t,i)=>t.classList.toggle('active',['keys','usage','audit','plugins'][i]===name));
  document.querySelectorAll('.tab-content').forEach(t=>t.classList.remove('active'));
  document.getElementById('tab-'+name).classList.add('active');
}

function toggleCreateKey(){
  const f=document.getElementById('create-key-form');
  f.style.display=f.style.display==='none'?'block':'none';
  document.getElementById('new-key-result').textContent='';
}

function createKey(){
  const alias=document.getElementById('new-alias').value;
  const rpm=parseInt(document.getElementById('new-rpm').value)||0;
  if(!alias)return;
  fetch('/admin/keys',{method:'POST',headers:{...H(),'Content-Type':'application/json'},
    body:JSON.stringify({alias,rate_limit_rpm:rpm})})
  .then(r=>r.json()).then(d=>{
    if(d.key){document.getElementById('new-key-result').textContent='Key: '+d.key;fetchKeys()}
    else document.getElementById('new-key-result').textContent='Error: '+(d.error||'unknown');
  });
}

function revokeKey(alias){
  if(!confirm('Revoke key "'+alias+'"?'))return;
  fetch('/admin/keys/'+alias,{method:'DELETE',headers:H()}).then(()=>fetchKeys());
}

function fmtDate(ts){if(!ts)return'-';const d=new Date(ts*1000);return d.toLocaleDateString()+' '+d.toLocaleTimeString()}
function fmtNum(n){if(n>=1e6)return(n/1e6).toFixed(1)+'M';if(n>=1e3)return(n/1e3).toFixed(1)+'K';return n}

function fetchMetrics(){
  fetch('/metrics',{headers:H()}).then(r=>r.json()).then(m=>{
    const up=m.uptime_seconds||0;const uh=Math.floor(up/3600);const um=Math.floor((up%3600)/60);
    document.getElementById('c-uptime').textContent=uh+'h '+um+'m';
    document.getElementById('c-requests').textContent=fmtNum(m.total_requests||0);
    document.getElementById('c-active').textContent=m.active_requests||0;
    const rss=m.memory_rss_kb||0;const peak=m.memory_peak_kb||0;
    document.getElementById('c-memory').textContent=rss>1024?(rss/1024).toFixed(1)+' MB':rss+' KB';
    document.getElementById('c-memory-sub').textContent='Peak: '+(peak>1024?(peak/1024).toFixed(1)+' MB':peak+' KB');
    const cache=m.cache||{};
    const hr=cache.hit_rate||0;
    document.getElementById('c-cache').textContent=(hr*100).toFixed(1)+'%';
    document.getElementById('c-cache-sub').textContent=(cache.entries||0)+' entries, '+(cache.evictions||0)+' evictions';
    const now=new Date().toLocaleTimeString();
    rpmData.push({t:now,v:m.total_requests||0});if(rpmData.length>30)rpmData.shift();
    latData.push({t:now,v:m.avg_latency_ms||0});if(latData.length>30)latData.shift();
    updateCharts();
  }).catch(()=>{});
}

function fetchHealth(){
  fetch('/health').then(r=>r.json()).then(d=>{
    document.getElementById('health-dot').className='dot'+(d.status==='ok'?'':' off');
    document.getElementById('health-text').textContent=d.status==='ok'?'Healthy':'Unhealthy';
  }).catch(()=>{
    document.getElementById('health-dot').className='dot off';
    document.getElementById('health-text').textContent='Offline';
  });
}

function fetchKeys(){
  fetch('/admin/keys',{headers:H()}).then(r=>r.json()).then(keys=>{
    const tb=document.getElementById('keys-body');tb.innerHTML='';
    (Array.isArray(keys)?keys:[]).forEach(k=>{
      const tr=document.createElement('tr');
      tr.innerHTML='<td>'+k.alias+'</td><td>'+k.key_prefix+'</td><td>'+fmtDate(k.created_at)+'</td>'
        +'<td>'+fmtDate(k.last_used_at)+'</td><td>'+fmtNum(k.request_count||0)+'</td>'
        +'<td>'+(k.rate_limit_rpm||'none')+'</td>'
        +'<td><span class="badge '+(k.active?'active':'revoked')+'">'+(k.active?'Active':'Revoked')+'</span></td>'
        +'<td>'+(k.active?'<button class="btn btn-danger btn-sm" onclick="revokeKey(\''+k.alias+'\')">Revoke</button>':'')+'</td>';
      tb.appendChild(tr);
    });
  }).catch(()=>{});
}

function fetchUsage(){
  fetch('/admin/usage',{headers:H()}).then(r=>r.json()).then(d=>{
    const tb=document.getElementById('usage-body');tb.innerHTML='';
    (d.keys||[]).forEach(k=>{
      const tr=document.createElement('tr');
      tr.innerHTML='<td>'+k.alias+'</td><td>'+fmtNum(k.daily_tokens)+'</td><td>'+fmtNum(k.daily_requests)+'</td>'
        +'<td>'+fmtNum(k.monthly_tokens)+'</td><td>'+fmtNum(k.monthly_requests)+'</td>';
      tb.appendChild(tr);
    });
  }).catch(()=>{});
}

function fetchAudit(){
  fetch('/admin/audit?limit=20',{headers:H()}).then(r=>r.json()).then(d=>{
    const tb=document.getElementById('audit-body');tb.innerHTML='';
    (d.entries||[]).forEach(e=>{
      const tr=document.createElement('tr');
      const st=e.status_code;
      const stcol=st>=500?'var(--red)':st>=400?'var(--yellow)':'var(--green)';
      tr.innerHTML='<td>'+fmtDate(e.timestamp)+'</td><td>'+(e.key_alias||'-')+'</td>'
        +'<td>'+e.method+'</td><td>'+(e.model||'-')+'</td>'
        +'<td style="color:'+stcol+'">'+st+'</td>'
        +'<td>'+(e.prompt_tokens+e.completion_tokens)+'</td>'
        +'<td>'+e.latency_ms+'ms</td>';
      tb.appendChild(tr);
    });
  }).catch(()=>{});
}

function fetchPlugins(){
  fetch('/admin/plugins',{headers:H()}).then(r=>r.json()).then(plugins=>{
    const tb=document.getElementById('plugins-body');tb.innerHTML='';
    (Array.isArray(plugins)?plugins:[]).forEach(p=>{
      const tr=document.createElement('tr');
      tr.innerHTML='<td>'+p.name+'</td><td>'+p.version+'</td><td>'+p.type+'</td><td>'+p.position+'</td>';
      tb.appendChild(tr);
    });
  }).catch(()=>{});
}

function initCharts(){
  const opts={responsive:true,plugins:{legend:{display:false}},scales:{x:{display:false},y:{beginAtZero:true,ticks:{color:'#94a3b8'},grid:{color:'#334155'}}}};
  rpmChart=new Chart(document.getElementById('chart-rpm'),{type:'line',data:{labels:[],datasets:[{data:[],borderColor:'#3b82f6',tension:.3,pointRadius:0,borderWidth:2}]},options:opts});
  latChart=new Chart(document.getElementById('chart-latency'),{type:'line',data:{labels:[],datasets:[{data:[],borderColor:'#eab308',tension:.3,pointRadius:0,borderWidth:2}]},options:opts});
}

function updateCharts(){
  if(!rpmChart)return;
  rpmChart.data.labels=rpmData.map(d=>d.t);
  rpmChart.data.datasets[0].data=rpmData.map((d,i)=>i>0?d.v-rpmData[i-1].v:0);
  rpmChart.update('none');
  latChart.data.labels=latData.map(d=>d.t);
  latChart.data.datasets[0].data=latData.map(d=>d.v);
  latChart.update('none');
}

function startPolling(){
  try{initCharts()}catch(e){}
  fetchMetrics();fetchHealth();fetchKeys();fetchUsage();fetchAudit();fetchPlugins();
  setInterval(()=>{fetchMetrics();fetchHealth()},5000);
  setInterval(()=>{fetchKeys();fetchUsage();fetchAudit()},15000);
}

window.onload=()=>{
  const saved=sessionStorage.getItem('hermes_key');
  if(saved){API_KEY=saved;document.getElementById('auth-modal').style.display='none';startPolling()}
  document.getElementById('key-input').addEventListener('keydown',e=>{if(e.key==='Enter')authenticate()});
};
</script>
</body>
</html>)html";
