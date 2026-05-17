#include "http_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "drivers/rtc/rtc.h"
#include "logic/extrusion.h"
#include "logic/production.h"
#include "logic/alarm_config.h"
#include "logic/profile.h"
#include "logic/active_profile.h"
#include "comms/wifi/wifi_manager.h"

static const char *TAG = "http";

// =============================================================
// HTML  (3 chunks sent via httpd_resp_sendstr_chunk)
// =============================================================

static const char HTML_HEAD[] =
"<!DOCTYPE html>"
"<html lang='es'><head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>DeepMove Controller</title>"
"<style>"
":root{--bg:#0d1117;--sb:#0a0f14;--card:#161b22;--card2:#1c2128;--brd:#30363d;"
"--txt:#e6edf3;--mut:#8b949e;--grn:#3fb950;--blu:#58a6ff;--org:#f78166;"
"--yel:#d29922;--red:#f85149}"
"*{box-sizing:border-box;margin:0;padding:0}"
"body{background:var(--bg);color:var(--txt);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;display:flex;height:100vh;overflow:hidden}"
".sb{width:190px;background:var(--sb);border-right:1px solid var(--brd);display:flex;flex-direction:column;flex-shrink:0}"
".logo{padding:16px;border-bottom:1px solid var(--brd)}"
".logo h1{font-size:14px;font-weight:700}.logo p{font-size:10px;color:var(--mut);margin-top:2px}"
"nav{flex:1;padding:6px 0;overflow-y:auto}"
".ns{font-size:10px;color:var(--mut);padding:10px 16px 4px;text-transform:uppercase;letter-spacing:.05em}"
".ni{display:flex;align-items:center;gap:10px;padding:8px 16px;cursor:pointer;font-size:13px;color:var(--mut);border-left:3px solid transparent;transition:all .15s;user-select:none}"
".ni:hover{color:var(--txt);background:rgba(255,255,255,.05)}"
".ni.act{color:var(--blu);background:rgba(88,166,255,.1);border-left-color:var(--blu)}"
".main{flex:1;display:flex;flex-direction:column;overflow:hidden}"
".topbar{padding:12px 20px;border-bottom:1px solid var(--brd);display:flex;align-items:center;justify-content:space-between;background:var(--sb);flex-shrink:0}"
".topbar h2{font-size:16px;font-weight:600}.topbar p{font-size:12px;color:var(--mut);margin-top:1px}"
".tbr{display:flex;align-items:center;gap:16px}"
".wbg{display:flex;align-items:center;gap:6px;font-size:12px}"
".dot{width:8px;height:8px;border-radius:50%;display:inline-block;flex-shrink:0}"
".dg{background:var(--grn)}.dr{background:var(--red)}"
".content{flex:1;overflow-y:auto;padding:14px}"
".grid{display:grid;gap:10px;margin-bottom:12px}"
".g4{grid-template-columns:repeat(4,1fr)}"
".g5{grid-template-columns:repeat(5,1fr)}"
".g21{grid-template-columns:2fr 1fr}"
".g12{grid-template-columns:1fr 1fr}"
".card{background:var(--card);border:1px solid var(--brd);border-radius:8px;padding:14px}"
".ct{font-size:11px;color:var(--mut);text-transform:uppercase;letter-spacing:.05em;margin-bottom:6px}"
".cv{font-size:24px;font-weight:700}.cs{font-size:11px;color:var(--mut);margin-top:3px}"
".cst{font-size:14px;font-weight:600}"
".cg{color:var(--grn)}.cb{color:var(--blu)}.co{color:var(--org)}.cy{color:var(--yel)}.cr{color:var(--red)}.cm{color:var(--mut)}"
".pb{height:5px;background:var(--card2);border-radius:3px;margin-top:6px;overflow:hidden}"
".pf{height:100%;border-radius:3px;transition:width .5s}"
".io{display:flex;align-items:center;justify-content:space-between;padding:7px 0;border-bottom:1px solid var(--brd);font-size:12px}"
".io:last-child{border-bottom:none}"
".bdg{font-size:11px;padding:2px 8px;border-radius:12px;font-weight:600}"
".bon{background:rgba(63,185,80,.15);color:var(--grn)}"
".bof{background:rgba(139,148,158,.15);color:var(--mut)}"
".bal{background:rgba(248,81,73,.15);color:var(--red)}"
".ali{padding:10px;background:rgba(248,81,73,.08);border:1px solid rgba(248,81,73,.2);border-radius:6px;margin-bottom:8px;font-size:12px}"
".alt{font-weight:600;color:var(--org);margin-bottom:2px}"
".sm{text-align:center}.sm .sv{font-size:16px;font-weight:700}.sm .sl{font-size:11px;color:var(--mut)}"
".view{display:none}.view.act{display:block}"
".plo{display:flex;gap:12px;height:calc(100vh - 128px)}"
".pll{width:260px;flex-shrink:0;display:flex;flex-direction:column}"
".ple{flex:1;overflow-y:auto}"
".sbi{width:100%;padding:7px 10px;background:var(--card2);border:1px solid var(--brd);border-radius:6px;color:var(--txt);font-size:13px;margin-bottom:8px;outline:none}"
".sbi:focus{border-color:var(--blu)}"
".pl{overflow-y:auto;flex:1}"
".pi{padding:9px 12px;border-radius:6px;cursor:pointer;margin-bottom:4px;border:1px solid var(--brd);background:var(--card);transition:all .15s}"
".pi:hover{border-color:var(--blu)}.pi.sel{border-color:var(--blu);background:rgba(88,166,255,.1)}"
".pic{font-size:13px;font-weight:600}.pin{font-size:11px;color:var(--mut);margin-top:1px}"
".fg{display:grid;grid-template-columns:1fr 1fr;gap:10px}"
".fgs{display:flex;flex-direction:column;gap:3px}"
".fgs label{font-size:11px;color:var(--mut);text-transform:uppercase}"
".fgs input,.fgs select{padding:7px 10px;background:var(--card2);border:1px solid var(--brd);border-radius:6px;color:var(--txt);font-size:13px;outline:none}"
".fgs input:focus,.fgs select:focus{border-color:var(--blu)}"
".fgs input:read-only{color:var(--mut);cursor:default}"
".fsec{font-size:11px;color:var(--mut);text-transform:uppercase;letter-spacing:.05em;margin:12px 0 6px;padding-bottom:4px;border-bottom:1px solid var(--brd)}"
".btn{padding:7px 14px;border:none;border-radius:6px;cursor:pointer;font-size:13px;font-weight:500;transition:all .15s}"
".bp{background:var(--blu);color:#000}.bp:hover{background:#79c0ff}"
".bd{background:rgba(248,81,73,.15);color:var(--red);border:1px solid rgba(248,81,73,.3)}.bd:hover{background:rgba(248,81,73,.25)}"
".bgh{background:transparent;color:var(--mut);border:1px solid var(--brd)}.bgh:hover{color:var(--txt)}"
".br{display:flex;gap:8px;margin-top:12px}"
".tabs{display:flex;border-bottom:1px solid var(--brd);margin-bottom:10px}"
".tab{padding:7px 14px;cursor:pointer;font-size:12px;color:var(--mut);border-bottom:2px solid transparent}"
".tab.act{color:var(--blu);border-bottom-color:var(--blu)}"
"table{width:100%;border-collapse:collapse;font-size:12px}"
"th{background:var(--card2);color:var(--mut);padding:8px 10px;text-align:left;font-size:11px;text-transform:uppercase}"
"td{padding:8px 10px;border-bottom:1px solid var(--brd)}"
"tr:hover td{background:rgba(255,255,255,.02)}"
"#speedChart{display:block;width:100%}"
"@media(max-width:1300px){.g5{grid-template-columns:repeat(3,1fr)}.g4{grid-template-columns:repeat(2,1fr)}}"
"</style></head>";

// ------------------------------------

static const char HTML_BODY[] =
"<body>"
"<div class='sb'>"
"  <div class='logo'><h1>&#9654; DeepMove</h1><p>CONTROLLER</p></div>"
"  <nav>"
"    <div class='ns'>Produccion</div>"
"    <div class='ni act' data-view='dashboard' onclick='nav(this)'>&#9632;&nbsp; Dashboard</div>"
"    <div class='ni' data-view='profiles' onclick='nav(this)'>&#9776;&nbsp; Perfiles</div>"
"    <div class='ni' data-view='logs' onclick='nav(this)'>&#9741;&nbsp; Logs</div>"
"  </nav>"
"  <div style='padding:10px 16px;font-size:11px;color:var(--mut);border-top:1px solid var(--brd)'>v1.0.0 &copy; 2025 Rubber SRL</div>"
"</div>"
"<div class='main'>"
"  <div class='topbar'>"
"    <div><h2 id='vtitle'>Dashboard</h2><p id='vsub'>Resumen general del sistema</p></div>"
"    <div class='tbr'>"
"      <div class='wbg'><div class='dot dr' id='wdot'></div><span id='wlbl'>Sin WiFi</span></div>"
"      <div style='font-size:11px;color:var(--mut)' id='uptime'>00:00:00</div>"
"    </div>"
"  </div>"
"  <div class='content'>"

/* === DASHBOARD VIEW === */
"  <div id='v-dashboard' class='view act'>"
"    <div class='grid g4'>"
"      <div class='card'><div class='ct'>Estado del sistema</div><div class='cst' id='sysst'>---</div><div class='cs' id='syssub'>&nbsp;</div></div>"
"      <div class='card'><div class='ct'>Perfil activo</div><div class='cst cb' id='aprof'>---</div><div class='cs' id='aprofsub'>&nbsp;</div></div>"
"      <div class='card'><div class='ct'>Modo</div><div class='cst co' id='mode'>Manual</div><div class='cs' id='modesub'>&nbsp;</div></div>"
"      <div class='card'><div class='ct'>IP local</div><div class='cst' id='ipdisp' style='font-family:monospace;font-size:13px'>---</div><div class='cs' id='ssiddisp'>&nbsp;</div></div>"
"    </div>"
"    <div class='grid g5'>"
"      <div class='card'><div class='ct'>Velocidad actual</div><div class='cv cg'><span id='spd'>0.00</span><span style='font-size:13px'> m/min</span></div><div class='cs' id='spdrng'>&nbsp;</div></div>"
"      <div class='card'><div class='ct'>Metros producidos</div><div class='cv cb'><span id='totm'>0.00</span><span style='font-size:13px'> m</span></div><div class='cs' id='totmsub'>&nbsp;</div></div>"
"      <div class='card'><div class='ct'>Cortes realizados</div><div class='cv co'><span id='cuts'>0</span><span style='font-size:13px'> cortes</span></div><div class='cs' id='cutssub'>&nbsp;</div></div>"
"      <div class='card'><div class='ct'>Long. de corte</div><div class='cv cy'><span id='cdist'>---</span><span style='font-size:13px'> m</span></div><div class='pb'><div class='pf' id='cprog' style='width:0%;background:var(--yel)'></div></div></div>"
"      <div class='card'><div class='ct'>Vel. promedio sesion</div><div class='cv'><span id='avgsp'>0.00</span><span style='font-size:13px'> m/min</span></div><div class='cs'>Sesion actual</div></div>"
"    </div>"
"    <div class='grid g21'>"
"      <div class='card'>"
"        <div style='display:flex;align-items:center;justify-content:space-between;margin-bottom:10px'>"
"          <span style='font-size:13px;font-weight:600'>Velocidad en tiempo real</span>"
"          <span style='font-size:11px;color:var(--mut)'><span style='color:var(--grn)'>&#8212; Actual</span>&nbsp;&nbsp;<span style='color:var(--org)'>- - Objetivo</span></span>"
"        </div>"
"        <canvas id='speedChart' height='170'></canvas>"
"        <div class='grid g4' style='margin-top:10px;margin-bottom:0'>"
"          <div class='sm'><div class='sv' id='st-avg'>0.00</div><div class='sl'>Promedio</div></div>"
"          <div class='sm'><div class='sv' id='st-max'>--</div><div class='sl'>Maxima</div></div>"
"          <div class='sm'><div class='sv' id='st-min'>--</div><div class='sl'>Minima</div></div>"
"          <div class='sm'><div class='sv' id='st-dev'>0.00</div><div class='sl'>Desv. Est.</div></div>"
"        </div>"
"      </div>"
"      <div class='card'>"
"        <div style='font-size:13px;font-weight:600;margin-bottom:10px'>Entradas / Salidas</div>"
"        <div class='tabs'>"
"          <div class='tab act' data-tab='out' onclick='iotab(this)'>Salidas</div>"
"          <div class='tab' data-tab='in' onclick='iotab(this)'>Entradas</div>"
"        </div>"
"        <div id='io-out'>"
"          <div class='io'><span>DO1 &ndash; Alarma</span><span class='bdg bof' id='io-al'>OFF</span></div>"
"          <div class='io'><span>DO2 &ndash; Corte (relay)</span><span class='bdg bof' id='io-rl'>OFF</span></div>"
"        </div>"
"        <div id='io-in' style='display:none'>"
"          <div class='io'><span>DI1 &ndash; Sensor correa</span><span class='bdg bon'>ON</span></div>"
"        </div>"
"        <div style='margin-top:14px'>"
"          <div style='font-size:13px;font-weight:600;margin-bottom:8px'>Alarmas activas</div>"
"          <div id='alml'><div style='font-size:12px;color:var(--mut)'>Sin alarmas activas</div></div>"
"        </div>"
"      </div>"
"    </div>"
"  </div>" /* /dashboard */

/* === PROFILES VIEW === */
"  <div id='v-profiles' class='view'>"
"    <div class='plo'>"
"      <div class='pll'>"
"        <div class='br' style='margin-top:0;margin-bottom:10px'>"
"          <button class='btn bp' style='flex:1' onclick='newProf()'>+ Nuevo</button>"
"          <button class='btn bgh' onclick='loadProfs()'>&#8635;</button>"
"        </div>"
"        <input class='sbi' id='psrch' placeholder='Buscar...' oninput='filterProfs()'/>"
"        <div class='pl' id='plist'></div>"
"      </div>"
"      <div class='ple card'>"
"        <div id='pempty' style='display:flex;align-items:center;justify-content:center;height:200px;color:var(--mut);font-size:13px'>Selecciona un perfil para editar</div>"
"        <div id='pform' style='display:none'>"
"          <div style='display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:8px'>"
"            <div style='font-size:15px;font-weight:600' id='pfmtitle'>Nuevo perfil</div>"
"            <div class='br' style='margin:0'>"
"              <button class='btn bgh' onclick='cancelEdit()'>Cancelar</button>"
"              <button class='btn bd' id='bdel' onclick='delProf()'>Eliminar</button>"
"              <button class='btn bp' onclick='saveProf()'>Guardar</button>"
"            </div>"
"          </div>"
"          <div class='fsec'>General</div>"
"          <div class='fg'>"
"            <div class='fgs'><label>Codigo (ID)</label><input id='f_code' placeholder='ej: 1000.5000'/></div>"
"            <div class='fgs'><label>Nombre comercial</label><input id='f_name' placeholder='ej: Manguera 10mm'/></div>"
"          </div>"
"          <div class='fsec'>Geometria</div>"
"          <div class='fg'>"
"            <div class='fgs'><label>Matriz</label><input id='f_matrix' placeholder='5000'/></div>"
"            <div class='fgs'><label>Bocas</label><input id='f_bocas' type='number' min='1' value='1'/></div>"
"            <div class='fgs'><label>Area (mm&sup2;)</label><input id='f_area' type='number' step='0.01' value='0'/></div>"
"          </div>"
"          <div class='fsec'>Proceso</div>"
"          <div class='fg'>"
"            <div class='fgs'><label>Husillo (%)</label><input id='f_screw' type='number' min='0' max='100' value='25'/></div>"
"            <div class='fgs'><label>RPM VFD objetivo</label><input id='f_vfd' type='number' value='450'/></div>"
"            <div class='fgs'><label>Vel. correa obj. (m/min)</label><input id='f_belt' type='number' step='0.1' value='8.0'/></div>"
"          </div>"
"          <div class='fsec'>Produccion</div>"
"          <div class='fg'>"
"            <div class='fgs'><label>Opciones de corte (m, separar con coma)</label><input id='f_cuts' placeholder='50, 100, 200'/></div>"
"            <div class='fgs'><label>Corte por defecto (m)</label><input id='f_defcut' type='number' step='0.5' value='50'/></div>"
"            <div class='fgs'><label>Permitir valor personalizado</label>"
"              <select id='f_custom'><option value='true'>Si</option><option value='false'>No</option></select>"
"            </div>"
"          </div>"
"          <div class='fsec'>Ingenieria</div>"
"          <div class='fg'>"
"            <div class='fgs'><label>Densidad teorica (gr/m)</label><input id='f_tden' type='number' step='0.01' value='0.20'/></div>"
"            <div class='fgs'><label>Densidad real (gr/m)</label><input id='f_rden' type='number' step='0.01' value='0.20'/></div>"
"          </div>"
"        </div>"
"      </div>"
"    </div>"
"  </div>" /* /profiles */

/* === LOGS VIEW === */
"  <div id='v-logs' class='view'>"
"    <div class='card'>"
"      <div style='display:flex;align-items:center;justify-content:space-between;margin-bottom:14px'>"
"        <span style='font-size:15px;font-weight:600'>Historico de produccion (hoy)</span>"
"        <button class='btn bgh' onclick='loadLogs()'>&#8635; Actualizar</button>"
"      </div>"
"      <div style='overflow-x:auto'>"
"        <table><thead><tr><th>Inicio</th><th>Fin</th><th>Perfil</th><th>Metros</th><th>Vel. prom.</th><th>Cortes</th><th>Motivo</th></tr></thead>"
"        <tbody id='ltbody'></tbody></table>"
"      </div>"
"      <div id='lempty' style='text-align:center;padding:40px;color:var(--mut);font-size:13px'>No hay registros para hoy</div>"
"    </div>"
"  </div>" /* /logs */

"  </div>" /* /content */
"</div>" /* /main */
"</body>";

// ------------------------------------

static const char HTML_JS[] =
"<script>"
/* --- VIEWS --- */
"const VIEWS={'dashboard':'Dashboard|Resumen general del sistema','profiles':'Perfiles|Gestion de perfiles','logs':'Logs de produccion|Historico del dia'};"
"function nav(el){"
"  document.querySelectorAll('.ni').forEach(n=>n.classList.remove('act'));"
"  el.classList.add('act');"
"  const id=el.dataset.view;"
"  document.querySelectorAll('.view').forEach(v=>v.classList.remove('act'));"
"  document.getElementById('v-'+id).classList.add('act');"
"  const[t,s]=VIEWS[id].split('|');"
"  document.getElementById('vtitle').textContent=t;"
"  document.getElementById('vsub').textContent=s;"
"  if(id==='profiles')loadProfs();"
"  if(id==='logs')loadLogs();"
"}"

/* --- UPTIME --- */
"const t0=Date.now();"
"setInterval(()=>{"
"  const s=Math.floor((Date.now()-t0)/1000);"
"  const h=String(Math.floor(s/3600)).padStart(2,'0');"
"  const m=String(Math.floor((s%3600)/60)).padStart(2,'0');"
"  document.getElementById('uptime').textContent=h+':'+m+':'+String(s%60).padStart(2,'0');"
"},1000);"

/* --- REAL-TIME DATA --- */
"let hist=[],hmax=-Infinity,hmin=Infinity,hsum=0,hn=0,tspd=0;"
"async function poll(){"
"  try{"
"    const r=await fetch('/api/status');"
"    if(!r.ok)return;"
"    const d=await r.json();"
"    update(d);"
"  }catch(e){}"
"}"
"function update(d){"
"  const spd=parseFloat(d.speed)||0;"
"  document.getElementById('spd').textContent=spd.toFixed(2);"
"  hist.push(spd);if(hist.length>180)hist.shift();"
"  if(spd>0){hmax=Math.max(hmax,spd);hmin=Math.min(hmin,spd);hsum+=spd;hn++;}"
"  const avg=hn>0?hsum/hn:0;"
"  const dev=stddev();"
"  document.getElementById('st-avg').textContent=avg.toFixed(2);"
"  document.getElementById('st-max').textContent=hmax===-Infinity?'--':hmax.toFixed(2);"
"  document.getElementById('st-min').textContent=hmin===Infinity?'--':hmin.toFixed(2);"
"  document.getElementById('st-dev').textContent=dev.toFixed(2);"
"  document.getElementById('avgsp').textContent=(parseFloat(d.avg_speed)||0).toFixed(2);"
"  document.getElementById('totm').textContent=(parseFloat(d.total_m)||0).toFixed(2);"
"  document.getElementById('cuts').textContent=d.cuts||0;"
"  document.getElementById('cutssub').textContent=parseInt(d.target_count)>0?'Objetivo: '+(d.cuts||0)+'/'+d.target_count:'Sin objetivo';"
"  const cd=parseFloat(d.cut_distance_m)||0;"
"  if(cd>0){"
"    document.getElementById('cdist').textContent=cd.toFixed(0);"
"    const tm=parseFloat(d.total_m)||0;"
"    const pct=Math.min(100,((tm%cd)/cd)*100);"
"    document.getElementById('cprog').style.width=pct+'%';"
"  }else{document.getElementById('cdist').textContent='---';}"
"  tspd=parseFloat(d.belt_speed)||0;"
"  if(tspd>0)document.getElementById('spdrng').textContent='Objetivo: '+tspd.toFixed(1)+' m/min';"
"  const rec=d.recording,alm=d.alarm_active;"
"  const st=document.getElementById('sysst');"
"  if(alm){st.textContent='ALARMA';st.className='cst cr';}"
"  else if(rec){st.textContent='Grabando';st.className='cst cg';}"
"  else{st.textContent='En espera';st.className='cst cm';}"
"  document.getElementById('mode').textContent=rec?'Automatico':'Manual';"
"  document.getElementById('modesub').textContent=cd>0?'Corte cada '+cd.toFixed(0)+' m':'Sin corte programado';"
"  document.getElementById('aprof').textContent=d.profile||'---';"
"  document.getElementById('aprofsub').textContent=d.profile?'Perfil seleccionado':'';"
"  const wok=d.wifi_connected;"
"  document.getElementById('wdot').className='dot '+(wok?'dg':'dr');"
"  document.getElementById('wlbl').textContent=wok?(d.wifi_ssid||'Conectado'):'Sin WiFi';"
"  document.getElementById('ipdisp').textContent=d.ip||'---';"
"  document.getElementById('ssiddisp').textContent=wok?'Red: '+(d.wifi_ssid||''):'Desconectado';"
"  const ral=document.getElementById('io-al'),rrl=document.getElementById('io-rl');"
"  ral.textContent=alm?'ON':'OFF';ral.className='bdg '+(alm?'bal':'bof');"
"  rrl.textContent='OFF';rrl.className='bdg bof';"
"  const al=document.getElementById('alml');"
"  if(alm)al.innerHTML='<div class=\"ali\"><div class=\"alt\">&#9888; Velocidad fuera de rango</div><div style=\"color:var(--mut)\">Actual: '+spd.toFixed(2)+' m/min &nbsp;|&nbsp; Objetivo: '+tspd.toFixed(2)+' m/min</div></div>';"
"  else al.innerHTML='<div style=\"font-size:12px;color:var(--mut)\">Sin alarmas activas</div>';"
"  drawChart();"
"}"
"function stddev(){"
"  if(hist.length<2)return 0;"
"  const mn=hist.reduce((a,b)=>a+b,0)/hist.length;"
"  return Math.sqrt(hist.map(v=>(v-mn)**2).reduce((a,b)=>a+b,0)/hist.length);"
"}"

/* --- CHART --- */
"function drawChart(){"
"  const cv=document.getElementById('speedChart');"
"  const W=cv.offsetWidth,H=170;"
"  cv.width=W;cv.height=H;"
"  const cx=cv.getContext('2d');"
"  const p={t:8,r:8,b:24,l:38};"
"  const cw=W-p.l-p.r,ch=H-p.t-p.b;"
"  cx.clearRect(0,0,W,H);"
"  if(hist.length<2){cx.fillStyle='#8b949e';cx.font='12px sans-serif';cx.textAlign='center';cx.fillText('Esperando datos...',W/2,H/2);return;}"
"  const mx=Math.max(...hist,tspd*1.2,0.5)*1.1,mn=0;"
"  const xp=i=>p.l+(i/(hist.length-1))*cw;"
"  const yp=v=>p.t+ch-((v-mn)/(mx-mn))*ch;"
"  cx.strokeStyle='#30363d';cx.lineWidth=1;"
"  for(let i=0;i<=4;i++){"
"    const v=mn+(mx-mn)*(i/4);const y=yp(v);"
"    cx.beginPath();cx.moveTo(p.l,y);cx.lineTo(p.l+cw,y);cx.stroke();"
"    cx.fillStyle='#8b949e';cx.font='10px sans-serif';cx.textAlign='right';"
"    cx.fillText(v.toFixed(1),p.l-3,y+4);"
"  }"
"  if(tspd>0){"
"    const ty=yp(tspd);"
"    cx.strokeStyle='#f78166';cx.lineWidth=1;cx.setLineDash([4,4]);"
"    cx.beginPath();cx.moveTo(p.l,ty);cx.lineTo(p.l+cw,ty);cx.stroke();"
"    cx.setLineDash([]);"
"  }"
"  cx.beginPath();cx.moveTo(xp(0),yp(hist[0]));"
"  for(let i=1;i<hist.length;i++)cx.lineTo(xp(i),yp(hist[i]));"
"  cx.lineTo(xp(hist.length-1),p.t+ch);cx.lineTo(xp(0),p.t+ch);cx.closePath();"
"  const g=cx.createLinearGradient(0,p.t,0,p.t+ch);"
"  g.addColorStop(0,'rgba(63,185,80,.35)');g.addColorStop(1,'rgba(63,185,80,0)');"
"  cx.fillStyle=g;cx.fill();"
"  cx.beginPath();cx.strokeStyle='#3fb950';cx.lineWidth=2;"
"  cx.moveTo(xp(0),yp(hist[0]));"
"  for(let i=1;i<hist.length;i++)cx.lineTo(xp(i),yp(hist[i]));"
"  cx.stroke();"
"  const now=new Date();"
"  cx.fillStyle='#8b949e';cx.font='10px sans-serif';cx.textAlign='center';"
"  for(let i=0;i<=4;i++){"
"    const idx=Math.floor(i*(hist.length-1)/4);"
"    const sec=(hist.length-1-idx)*2;"
"    const t=new Date(now-sec*1000);"
"    cx.fillText(t.getHours().toString().padStart(2,'0')+':'+t.getMinutes().toString().padStart(2,'0'),xp(idx),H-6);"
"  }"
"}"

/* --- IO TABS --- */
"function iotab(el){"
"  document.querySelectorAll('.tab').forEach(t=>t.classList.remove('act'));"
"  el.classList.add('act');"
"  const t=el.dataset.tab;"
"  document.getElementById('io-out').style.display=t==='out'?'':'none';"
"  document.getElementById('io-in').style.display=t==='in'?'':'none';"
"}"

/* --- PROFILES --- */
"let allCodes=[],curCode=null,isNew=false;"
"async function loadProfs(){"
"  try{"
"    const r=await fetch('/api/profiles');"
"    if(!r.ok)return;"
"    allCodes=await r.json();"
"    renderList(allCodes);"
"  }catch(e){console.error(e);}"
"}"
"function renderList(codes){"
"  const el=document.getElementById('plist');el.innerHTML='';"
"  if(!codes.length){el.innerHTML='<div style=\"color:var(--mut);font-size:12px;padding:8px\">Sin perfiles</div>';return;}"
"  codes.forEach(c=>{"
"    const d=document.createElement('div');"
"    d.className='pi'+(curCode===c?' sel':'');"
"    d.innerHTML='<div class=\"pic\">'+c+'</div>';"
"    d.onclick=()=>openProf(c,d);"
"    el.appendChild(d);"
"  });"
"}"
"function filterProfs(){"
"  const q=document.getElementById('psrch').value.toLowerCase();"
"  renderList(allCodes.filter(c=>c.toLowerCase().includes(q)));"
"}"
"async function openProf(code,el){"
"  document.querySelectorAll('.pi').forEach(e=>e.classList.remove('sel'));"
"  if(el)el.classList.add('sel');"
"  try{"
"    const r=await fetch('/api/profile?code='+encodeURIComponent(code));"
"    if(!r.ok)return;"
"    const data=await r.json();"
"    curCode=code;isNew=false;"
"    fillForm(code,data);"
"  }catch(e){console.error(e);}"
"}"
"function fillForm(code,d){"
"  const g=d.general||{},geo=d.geometry||{},pr=d.production||{},pc=d.process||{},en=d.engineering||{};"
"  document.getElementById('f_code').value=code;document.getElementById('f_code').readOnly=true;"
"  document.getElementById('f_name').value=g.commercial_name||'';"
"  document.getElementById('f_matrix').value=geo.matrix||'';"
"  document.getElementById('f_bocas').value=geo.bocas||1;"
"  document.getElementById('f_area').value=geo.area_mm2||0;"
"  document.getElementById('f_screw').value=pc.screw||25;"
"  document.getElementById('f_vfd').value=pc.target_speed_vfd_rpm||450;"
"  document.getElementById('f_belt').value=pc.target_speed_belt_m_min||8.0;"
"  document.getElementById('f_cuts').value=(pr.cut_options_m||[]).join(', ');"
"  document.getElementById('f_defcut').value=pr.default_cut_m||50;"
"  document.getElementById('f_custom').value=pr.allow_custom!==false?'true':'false';"
"  document.getElementById('f_tden').value=en.theoretical_density_gr_m||0.20;"
"  document.getElementById('f_rden').value=en.real_density_gr_m||0.20;"
"  document.getElementById('pfmtitle').textContent='Editando: '+code;"
"  document.getElementById('bdel').style.display='';"
"  document.getElementById('pempty').style.display='none';"
"  document.getElementById('pform').style.display='';"
"}"
"function newProf(){"
"  curCode=null;isNew=true;"
"  document.getElementById('f_code').value='';document.getElementById('f_code').readOnly=false;"
"  ['f_name','f_matrix'].forEach(id=>document.getElementById(id).value='');"
"  document.getElementById('f_bocas').value='1';"
"  document.getElementById('f_area').value='0';"
"  document.getElementById('f_screw').value='25';"
"  document.getElementById('f_vfd').value='450';"
"  document.getElementById('f_belt').value='8.0';"
"  document.getElementById('f_cuts').value='50, 100';"
"  document.getElementById('f_defcut').value='50';"
"  document.getElementById('f_custom').value='true';"
"  document.getElementById('f_tden').value='0.20';"
"  document.getElementById('f_rden').value='0.20';"
"  document.getElementById('pfmtitle').textContent='Nuevo perfil';"
"  document.getElementById('bdel').style.display='none';"
"  document.getElementById('pempty').style.display='none';"
"  document.getElementById('pform').style.display='';"
"  document.querySelectorAll('.pi').forEach(e=>e.classList.remove('sel'));"
"}"
"function cancelEdit(){"
"  curCode=null;isNew=false;"
"  document.getElementById('pempty').style.display='';"
"  document.getElementById('pform').style.display='none';"
"  document.querySelectorAll('.pi').forEach(e=>e.classList.remove('sel'));"
"}"
"function buildJSON(code){"
"  const cuts=document.getElementById('f_cuts').value.split(',').map(s=>parseFloat(s.trim())).filter(v=>!isNaN(v));"
"  return JSON.stringify({"
"    general:{code:code,commercial_name:document.getElementById('f_name').value},"
"    geometry:{matrix:document.getElementById('f_matrix').value,bocas:parseInt(document.getElementById('f_bocas').value),area_mm2:parseFloat(document.getElementById('f_area').value)},"
"    production:{cut_options_m:cuts,default_cut_m:parseFloat(document.getElementById('f_defcut').value),allow_custom:document.getElementById('f_custom').value==='true'},"
"    process:{screw:parseInt(document.getElementById('f_screw').value),target_speed_vfd_rpm:parseInt(document.getElementById('f_vfd').value),target_speed_belt_m_min:parseFloat(document.getElementById('f_belt').value)},"
"    engineering:{theoretical_density_gr_m:parseFloat(document.getElementById('f_tden').value),real_density_gr_m:parseFloat(document.getElementById('f_rden').value)},"
"    files:{image:''}"
"  },null,2);"
"}"
"async function saveProf(){"
"  const code=document.getElementById('f_code').value.trim();"
"  if(!code){alert('El codigo es requerido');return;}"
"  try{"
"    const r=await fetch('/api/profile?code='+encodeURIComponent(code),{method:'POST',body:buildJSON(code),headers:{'Content-Type':'application/json'}});"
"    if(r.ok){alert('Perfil guardado');curCode=code;isNew=false;loadProfs();}"
"    else alert('Error al guardar: '+r.status);"
"  }catch(e){alert('Error: '+e);}"
"}"
"async function delProf(){"
"  if(!curCode)return;"
"  if(!confirm('Eliminar perfil '+curCode+'?'))return;"
"  try{"
"    const r=await fetch('/api/profile?code='+encodeURIComponent(curCode),{method:'DELETE'});"
"    if(r.ok){alert('Eliminado');cancelEdit();loadProfs();}"
"    else alert('No se pudo eliminar (perfil activo o error)');"
"  }catch(e){}"
"}"

/* --- LOGS --- */
"async function loadLogs(){"
"  try{"
"    const r=await fetch('/api/logs');"
"    const tb=document.getElementById('ltbody'),em=document.getElementById('lempty');"
"    if(!r.ok){tb.innerHTML='';em.style.display='';return;}"
"    const txt=await r.text();"
"    const lines=txt.trim().split('\\n').filter(l=>l.trim());"
"    if(!lines.length){tb.innerHTML='';em.style.display='';return;}"
"    em.style.display='none';"
"    tb.innerHTML=lines.map(l=>'<tr>'+l.split(',').map(c=>'<td>'+c.trim()+'</td>').join('')+'</tr>').join('');"
"  }catch(e){}"
"}"

/* --- INIT --- */
"poll();"
"setInterval(poll,2000);"
"window.addEventListener('resize',drawChart);"
"</script></html>";

// =============================================================
// /api/status  — datos en tiempo real
// =============================================================
static esp_err_t status_handler(httpd_req_t *req)
{
    float speed     = extrusion_get_speed_m_min();
    float total_m   = extrusion_get_total_mm() / 1000.0f;
    int   cuts      = extrusion_get_total_count();
    float cut_dist  = extrusion_get_cut_distance_m();
    float avg_speed = extrusion_get_avg_speed();
    bool  recording = production_is_running();
    int   tgt_count = extrusion_get_target_count();
    const char *start_t = extrusion_get_start_time();
    const char *active  = active_profile_get();
    bool wifi_ok = wifi_is_connected();
    const char *ip   = wifi_get_ip_string();
    const char *ssid = wifi_get_ssid();

    // Belt speed + alarm state from active profile
    float belt_speed  = 0.0f;
    bool  alarm_active = false;
    if (active && strlen(active) > 0) {
        profile_t p;
        if (profile_get_by_code(active, &p)) {
            belt_speed = p.belt_speed;
            if (alarm_config_is_enabled() && belt_speed > 0.0f && recording) {
                float diff = speed > belt_speed ? speed - belt_speed : belt_speed - speed;
                float dev  = diff / belt_speed * 100.0f;
                alarm_active = (dev >= (float)alarm_config_get_threshold());
            }
        }
    }

    char *buf = malloc(512);
    if (!buf) return ESP_FAIL;

    snprintf(buf, 512,
        "{"
        "\"speed\":%.2f,"
        "\"avg_speed\":%.2f,"
        "\"total_m\":%.2f,"
        "\"cuts\":%d,"
        "\"cut_distance_m\":%.1f,"
        "\"target_count\":%d,"
        "\"recording\":%s,"
        "\"alarm_active\":%s,"
        "\"profile\":\"%s\","
        "\"belt_speed\":%.2f,"
        "\"start_time\":\"%s\","
        "\"wifi_connected\":%s,"
        "\"wifi_ssid\":\"%s\","
        "\"ip\":\"%s\""
        "}",
        speed, avg_speed, total_m, cuts, cut_dist, tgt_count,
        recording    ? "true" : "false",
        alarm_active ? "true" : "false",
        active   ? active   : "",
        belt_speed,
        start_t  ? start_t  : "",
        wifi_ok  ? "true" : "false",
        ssid     ? ssid     : "",
        ip       ? ip       : ""
    );

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
    free(buf);
    return ESP_OK;
}

// =============================================================
// /api/profiles  — lista de códigos
// =============================================================
static esp_err_t profiles_list_handler(httpd_req_t *req)
{
    char results[30][32];
    int count = profile_search(NULL, results, 30);

    int json_size = count * 38 + 8;
    char *json = malloc(json_size);
    if (!json)
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No mem");

    strcpy(json, "[");
    for (int i = 0; i < count; i++) {
        if (i > 0) strcat(json, ",");
        strcat(json, "\"");
        strcat(json, results[i]);
        strcat(json, "\"");
    }
    strcat(json, "]");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    free(json);
    return ESP_OK;
}

// =============================================================
// /api/profile?code=X   GET / POST / DELETE
// =============================================================
static esp_err_t profile_get_handler(httpd_req_t *req)
{
    char query[128], code[64], filepath[160];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta query");

    if (httpd_query_key_value(query, "code", code, sizeof(code)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta code");

    snprintf(filepath, sizeof(filepath), "/sdcard/profiles/%s.json", code);

    FILE *f = fopen(filepath, "r");
    if (!f)
        return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Perfil no encontrado");

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buf = malloc(size + 1);
    if (!buf) { fclose(f); return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No mem"); }

    fread(buf, 1, size, f);
    buf[size] = 0;
    fclose(f);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
    free(buf);
    return ESP_OK;
}

static esp_err_t profile_save_handler(httpd_req_t *req)
{
    char query[128], code[64], filepath[160];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta query");

    if (httpd_query_key_value(query, "code", code, sizeof(code)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta code");

    snprintf(filepath, sizeof(filepath), "/sdcard/profiles/%s.json", code);

    int total = req->content_len;
    if (total <= 0 || total > 4096)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Body invalido");

    char *body = malloc(total + 1);
    if (!body) return ESP_FAIL;

    int received = 0;
    while (received < total) {
        int n = httpd_req_recv(req, body + received, total - received);
        if (n <= 0) { free(body); return ESP_FAIL; }
        received += n;
    }
    body[total] = 0;

    FILE *f = fopen(filepath, "w");
    if (!f) { free(body); return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error al guardar"); }

    fwrite(body, 1, strlen(body), f);
    fclose(f);
    free(body);

    ESP_LOGI(TAG, "Perfil guardado: %s", filepath);
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

static esp_err_t profile_delete_handler(httpd_req_t *req)
{
    char query[128], code[64];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta query");

    if (httpd_query_key_value(query, "code", code, sizeof(code)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta code");

    if (!profile_delete(code))
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No se pudo eliminar");

    httpd_resp_sendstr(req, "Deleted");
    return ESP_OK;
}

// =============================================================
// /api/logs  — CSV del día
// =============================================================
static esp_err_t logs_handler(httpd_req_t *req)
{
    char date[32], filepath[128];
    rtc_get_date_filename_string(date);
    snprintf(filepath, sizeof(filepath), "/sdcard/logs/production_%s.csv", date);

    FILE *f = fopen(filepath, "r");
    if (!f)
        return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "CSV no encontrado");

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buf = malloc(size + 1);
    if (!buf) { fclose(f); return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No mem"); }

    fread(buf, 1, size, f);
    buf[size] = 0;
    fclose(f);

    httpd_resp_set_type(req, "text/csv");
    httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
    free(buf);
    return ESP_OK;
}

// =============================================================
// /   — dashboard HTML (3 chunks)
// =============================================================
static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr_chunk(req, HTML_HEAD);
    httpd_resp_sendstr_chunk(req, HTML_BODY);
    httpd_resp_sendstr_chunk(req, HTML_JS);
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

// =============================================================
// START SERVER
// =============================================================
void start_http_server(void)
{
    httpd_config_t config  = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 10;
    config.stack_size       = 8192;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Error iniciando servidor HTTP");
        return;
    }

    httpd_uri_t routes[] = {
        { .uri = "/",             .method = HTTP_GET,    .handler = root_get_handler    },
        { .uri = "/api/status",   .method = HTTP_GET,    .handler = status_handler      },
        { .uri = "/api/profiles", .method = HTTP_GET,    .handler = profiles_list_handler },
        { .uri = "/api/profile",  .method = HTTP_GET,    .handler = profile_get_handler  },
        { .uri = "/api/profile",  .method = HTTP_POST,   .handler = profile_save_handler },
        { .uri = "/api/profile",  .method = HTTP_DELETE, .handler = profile_delete_handler },
        { .uri = "/api/logs",     .method = HTTP_GET,    .handler = logs_handler         },
    };

    for (int i = 0; i < (int)(sizeof(routes) / sizeof(routes[0])); i++)
        httpd_register_uri_handler(server, &routes[i]);

    ESP_LOGI(TAG, "HTTP server started");
}
