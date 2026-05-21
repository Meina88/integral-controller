#include "http_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
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
":root{--bg:#0d1117;--sb:#010409;--card:#161b22;--card2:#1c2128;--brd:#30363d;"
"--txt:#e6edf3;--mut:#8b949e;--grn:#00e676;--grn2:rgba(0,230,118,.08);--grn3:rgba(0,230,118,.18);"
"--blu:#58a6ff;--org:#f78166;--yel:#d29922;--red:#f85149}"
"*{box-sizing:border-box;margin:0;padding:0}"
"body{background:var(--bg);color:var(--txt);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;display:flex;height:100vh;overflow:hidden}"
".sb{width:190px;background:var(--sb);border-right:1px solid var(--brd);display:flex;flex-direction:column;flex-shrink:0;box-shadow:inset 0 -120px 120px rgba(0,230,118,.08)}"
".logo{padding:16px;border-bottom:1px solid var(--brd)}"
".logo h1{font-size:14px;font-weight:700;color:var(--grn)}.logo p{font-size:10px;color:var(--mut);margin-top:2px}"
"nav{flex:1;padding:6px 0;overflow-y:auto}"
".ns{font-size:10px;color:var(--grn);padding:10px 16px 4px;text-transform:uppercase;letter-spacing:.1em;font-weight:700}"
".ni{display:flex;align-items:center;gap:10px;padding:8px 16px;cursor:pointer;font-size:13px;color:var(--mut);border-left:3px solid transparent;transition:all .15s;user-select:none}"
".ni:hover{color:var(--txt);background:var(--grn2)}"
".ni.act{color:var(--grn);background:var(--grn2);border-left-color:var(--grn);text-shadow:0 0 8px rgba(0,230,118,.4)}"
".main{flex:1;display:flex;flex-direction:column;overflow:hidden}"
".topbar{padding:12px 20px;border-bottom:1px solid var(--brd);display:flex;align-items:center;justify-content:space-between;background:var(--sb);flex-shrink:0}"
".topbar h2{font-size:16px;font-weight:600}.topbar p{font-size:12px;color:var(--mut);margin-top:1px}"
".tbr{display:flex;align-items:center;gap:16px}"
".wbg{display:flex;align-items:center;gap:6px;font-size:12px}"
".dot{width:8px;height:8px;border-radius:50%;display:inline-block;flex-shrink:0}"
".dg{background:var(--grn);box-shadow:0 0 8px var(--grn)}.dr{background:var(--red)}"
".content{flex:1;overflow-y:auto;padding:14px}"
".grid{display:grid;gap:10px;margin-bottom:12px}"
".g4{grid-template-columns:repeat(4,1fr)}"
".g5{grid-template-columns:repeat(5,1fr)}"
".g21{grid-template-columns:2fr 1fr}"
".g12{grid-template-columns:1fr 1fr}"
".card{background:var(--card);border:1px solid var(--brd);border-radius:8px;padding:14px}"
".ch{display:flex;align-items:flex-start;justify-content:space-between;margin-bottom:6px}"
".ct{font-size:11px;color:var(--mut);text-transform:uppercase;letter-spacing:.05em}"
".ci{width:34px;height:34px;border-radius:50%;background:var(--grn2);border:1px solid var(--grn3);display:flex;align-items:center;justify-content:center;font-size:15px;color:var(--grn);flex-shrink:0}"
".cv{font-size:26px;font-weight:700;color:var(--grn)}"
".cu{font-size:12px;font-weight:400;color:var(--mut);margin-left:3px}"
".cs{font-size:11px;color:var(--mut);margin-top:3px}"
".cst{font-size:14px;font-weight:600}"
".cg{color:var(--grn)}.cb{color:var(--blu)}.co{color:var(--org)}.cy{color:var(--yel)}.cr{color:var(--red)}.cm{color:var(--mut)}"
".pb{height:3px;background:rgba(0,230,118,.12);border-radius:2px;margin-top:10px;overflow:hidden}"
".pf{height:100%;border-radius:2px;transition:width .6s ease;background:var(--grn);box-shadow:0 0 6px rgba(0,230,118,.5)}"
".io{display:flex;align-items:center;justify-content:space-between;padding:7px 0;border-bottom:1px solid var(--brd);font-size:12px}"
".io:last-child{border-bottom:none}"
".bdg{font-size:11px;padding:2px 8px;border-radius:12px;font-weight:600}"
".bon{background:rgba(0,230,118,.15);color:var(--grn)}"
".bof{background:rgba(139,148,158,.12);color:var(--mut)}"
".bal{background:rgba(248,81,73,.15);color:var(--red)}"
".ali{padding:10px;background:rgba(248,81,73,.08);border:1px solid rgba(248,81,73,.2);border-radius:6px;margin-bottom:8px;font-size:12px}"
".alt{font-weight:600;color:var(--org);margin-bottom:2px}"
".sm{text-align:center}.sm .sv{font-size:16px;font-weight:700;color:var(--grn)}.sm .sl{font-size:11px;color:var(--mut)}"
".view{display:none}.view.act{display:block}"
".plo{display:flex;gap:12px;height:calc(100vh - 128px)}"
".pll{width:260px;flex-shrink:0;display:flex;flex-direction:column}"
".ple{flex:1;overflow-y:auto}"
".sbi{width:100%;padding:7px 10px;background:var(--card2);border:1px solid var(--brd);border-radius:6px;color:var(--txt);font-size:13px;margin-bottom:8px;outline:none}"
".sbi:focus{border-color:var(--grn);box-shadow:0 0 0 2px rgba(0,230,118,.15)}"
".pl{overflow-y:auto;flex:1}"
".pi{padding:9px 12px;border-radius:6px;cursor:pointer;margin-bottom:4px;border:1px solid var(--brd);background:var(--card);transition:all .15s}"
".pi:hover{border-color:var(--grn);background:var(--grn2)}.pi.sel{border-color:var(--grn);background:var(--grn2)}"
".pic{font-size:13px;font-weight:600}.pin{font-size:11px;color:var(--mut);margin-top:1px}"
".fg{display:grid;grid-template-columns:1fr 1fr;gap:10px}"
".fgs{display:flex;flex-direction:column;gap:3px}"
".fgs label{font-size:11px;color:var(--mut);text-transform:uppercase}"
".fgs input,.fgs select{padding:7px 10px;background:var(--card2);border:1px solid var(--brd);border-radius:6px;color:var(--txt);font-size:13px;outline:none}"
".fgs input:focus,.fgs select:focus{border-color:var(--grn);box-shadow:0 0 0 2px rgba(0,230,118,.12)}"
".fgs input:read-only{color:var(--mut);cursor:default}"
".fsec{font-size:11px;color:var(--grn);text-transform:uppercase;letter-spacing:.06em;margin:12px 0 6px;padding-bottom:4px;border-bottom:1px solid var(--brd)}"
".btn{padding:7px 14px;border:none;border-radius:6px;cursor:pointer;font-size:13px;font-weight:600;transition:all .15s}"
".bp{background:var(--grn);color:#000}.bp:hover{filter:brightness(1.12);box-shadow:0 0 12px rgba(0,230,118,.4)}"
".bd{background:rgba(248,81,73,.15);color:var(--red);border:1px solid rgba(248,81,73,.3)}.bd:hover{background:rgba(248,81,73,.25)}"
".bgh{background:transparent;color:var(--mut);border:1px solid var(--brd)}.bgh:hover{color:var(--txt);border-color:var(--grn)}"
".br{display:flex;gap:8px;margin-top:12px}"
".tabs{display:flex;border-bottom:1px solid var(--brd);margin-bottom:10px}"
".tab{padding:7px 14px;cursor:pointer;font-size:12px;color:var(--mut);border-bottom:2px solid transparent}"
".tab.act{color:var(--grn);border-bottom-color:var(--grn)}"
"table{width:100%;border-collapse:collapse;font-size:12px}"
"th{background:var(--card2);color:var(--grn);padding:8px 10px;text-align:left;font-size:10px;text-transform:uppercase;letter-spacing:.06em}"
"td{padding:8px 10px;border-bottom:1px solid var(--brd)}"
"tr:hover td{background:var(--grn2)}"
"#speedChart{display:block;width:100%}"
"@media(max-width:1300px){.g5{grid-template-columns:repeat(3,1fr)}.g4{grid-template-columns:repeat(2,1fr)}.g3{grid-template-columns:repeat(2,1fr)}}"
".g3{grid-template-columns:repeat(3,1fr)}"
".mx{width:26px;height:26px;border-radius:6px;background:rgba(139,148,158,.08);border:1px solid var(--brd);display:flex;align-items:center;justify-content:center;font-size:13px;color:var(--mut);cursor:pointer;flex-shrink:0;transition:all .15s}"
".mx:hover{color:var(--grn);border-color:rgba(0,230,118,.4);background:rgba(0,230,118,.06)}"
".fmo{display:none;position:fixed;inset:0;background:rgba(0,0,0,.82);z-index:1000;align-items:center;justify-content:center}"
".fmo.open{display:flex}"
".fmc{background:var(--card);border:1px solid var(--brd);border-radius:10px;width:88vw;max-width:1200px;height:88vh;display:flex;flex-direction:column;overflow:hidden}"
".fmh{display:flex;align-items:center;justify-content:space-between;padding:14px 20px;border-bottom:1px solid var(--brd);flex-shrink:0}"
".fmh h3{font-size:16px;font-weight:600}"
".fmb{flex:1;overflow:auto;padding:24px;display:flex;flex-direction:column;align-items:center;justify-content:center}"
".fmv{font-size:72px;font-weight:700;color:var(--grn);line-height:1}"
".fmu{font-size:18px;color:var(--mut);margin-top:8px}"
".ltab-wrap{display:flex;border-bottom:1px solid var(--brd);margin-bottom:14px}"
".ltab{padding:9px 18px;cursor:pointer;font-size:13px;font-weight:500;color:var(--mut);border-bottom:2px solid transparent;transition:color .15s}"
".ltab.act{color:var(--grn);border-bottom-color:var(--grn)}"
".ltab:hover{color:var(--txt)}"
".lpanel{display:none}.lpanel.act{display:block}"
".lsg{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin-bottom:0}"
".lst{background:var(--card2);border:1px solid var(--brd);border-radius:8px;padding:14px}"
".lsv{font-size:26px;font-weight:700;color:var(--grn)}.lsl{font-size:11px;color:var(--mut);text-transform:uppercase;margin-top:4px}"
".lcanv{display:block;width:100%}"
".psel{padding:6px 12px;background:var(--card2);border:1px solid var(--brd);border-radius:6px;color:var(--txt);font-size:13px;outline:none}"
".psel:focus{border-color:var(--grn)}"
"@media(max-width:900px){.lsg{grid-template-columns:repeat(2,1fr)}}"
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

/* Top 4 status cards */
"    <div class='grid g3'>"
"      <div class='card'><div class='ch'><div class='ct'>Estado del sistema</div><div class='mx' onclick='maxCard(\"status\")'>&#x26F6;</div></div><div class='cst' id='sysst'>---</div><div class='cs' id='syssub'>&nbsp;</div></div>"
"      <div class='card'><div class='ch'><div class='ct'>Perfil activo</div><div class='mx' onclick='maxCard(\"profile\")'>&#x26F6;</div></div><div class='cst cg' id='aprof'>---</div><div class='cs' id='aprofsub'>&nbsp;</div></div>"
"      <div class='card'><div class='ch'><div class='ct'>IP local</div><div class='mx' onclick='maxCard(\"ip\")'>&#x26F6;</div></div><div class='cst' id='ipdisp' style='font-family:monospace;font-size:13px;color:var(--grn)'>---</div><div class='cs' id='ssiddisp'>&nbsp;</div></div>"
"    </div>"

/* 5 metric cards */
"    <div class='grid g5'>"
"      <div class='card'><div class='ch'><div class='ct'>Velocidad actual</div><div class='mx' onclick='maxCard(\"spd\")'>&#x26F6;</div></div><div class='cv'><span id='spd'>0.00</span><span class='cu'>m/min</span></div><div class='cs' id='spdrng'>&nbsp;</div><div class='pb'><div class='pf' id='spdpb' style='width:0%'></div></div></div>"
"      <div class='card'><div class='ch'><div class='ct'>Metros producidos</div><div class='mx' onclick='maxCard(\"totm\")'>&#x26F6;</div></div><div class='cv'><span id='totm'>0.00</span><span class='cu'>m</span></div><div class='cs' id='totmsub'>&nbsp;</div><div class='pb'><div class='pf' id='totmpb' style='width:0%'></div></div></div>"
"      <div class='card'><div class='ch'><div class='ct'>Cortes realizados</div><div class='mx' onclick='maxCard(\"cuts\")'>&#x26F6;</div></div><div class='cv'><span id='cuts'>0</span><span class='cu'>cortes</span></div><div class='cs' id='cutssub'>&nbsp;</div><div class='pb'><div class='pf' id='cutspb' style='width:0%'></div></div></div>"
"      <div class='card'><div class='ch'><div class='ct'>Long. de corte</div><div class='mx' onclick='maxCard(\"cdist\")'>&#x26F6;</div></div><div class='cv'><span id='cdist'>---</span><span class='cu'>m</span></div><div class='pb'><div class='pf' id='cprog' style='width:0%'></div></div></div>"
"      <div class='card'><div class='ch'><div class='ct'>Vel. promedio sesion</div><div class='mx' onclick='maxCard(\"avgsp\")'>&#x26F6;</div></div><div class='cv'><span id='avgsp'>0.00</span><span class='cu'>m/min</span></div><div class='cs'>Sesion actual</div><div class='pb'><div class='pf' id='avgpb' style='width:0%'></div></div></div>"
"    </div>"

/* Chart + IO row */
"    <div class='grid g21'>"
"      <div class='card'>"
"        <div style='display:flex;align-items:center;justify-content:space-between;margin-bottom:10px'>"
"          <span style='font-size:13px;font-weight:600'>Velocidad en tiempo real</span>"
"          <div style='display:flex;align-items:center;gap:10px'><span style='font-size:11px;color:var(--mut)'><span style='color:var(--grn)'>&#8212; Actual</span>&nbsp;&nbsp;<span style='color:var(--org)'>- - Objetivo</span></span><div class='mx' onclick='maxCard(\"chart\")'>&#x26F6;</div></div>"
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
"        <div style='display:flex;align-items:center;justify-content:space-between;margin-bottom:10px'><span style='font-size:13px;font-weight:600'>Entradas / Salidas</span><div class='mx' onclick='maxCard(\"io\")'>&#x26F6;</div></div>"
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
"    <div class='ltab-wrap'>"
"      <div class='ltab act' data-lt='profile' onclick='ltab(this)'>Por Perfil</div>"
"      <div class='ltab' data-lt='period' onclick='ltab(this)'>Por Periodo</div>"
"      <div class='ltab' data-lt='stats' onclick='ltab(this)'>Estadisticas</div>"
"    </div>"
"    <div id='lt-profile' class='lpanel act card'>"
"      <div style='display:flex;align-items:center;justify-content:space-between;margin-bottom:12px'>"
"        <span style='font-size:13px;font-weight:600'>Produccion por perfil</span>"
"        <button class='btn bgh' onclick='loadAllLogs()'>&#8635; Actualizar</button>"
"      </div>"
"      <select class='psel' id='lp-sel' onchange='renderProfileTab()'><option value=''>Todos los perfiles</option></select>"
"      <div id='lp-stats' class='lsg' style='margin-top:12px'></div>"
"      <canvas class='lcanv' id='lp-chart' height='200' style='margin-top:10px'></canvas>"
"      <div id='lp-empty' style='text-align:center;padding:30px;color:var(--mut);font-size:13px'>Cargando...</div>"
"    </div>"
"    <div id='lt-period' class='lpanel card'>"
"      <div style='display:flex;align-items:center;justify-content:space-between;margin-bottom:12px'>"
"        <select class='psel' id='period-sel' onchange='renderPeriodTab()'>"
"          <option value='day'>Hoy</option>"
"          <option value='week'>Esta semana</option>"
"          <option value='month' selected>Este mes</option>"
"          <option value='year'>Este a&ntilde;o</option>"
"        </select>"
"        <button class='btn bgh' onclick='loadAllLogs()'>&#8635; Actualizar</button>"
"      </div>"
"      <div id='lper-stats' class='lsg'></div>"
"      <canvas class='lcanv' id='lper-chart' height='200' style='margin-top:10px'></canvas>"
"      <div id='lper-empty' style='text-align:center;padding:30px;color:var(--mut);font-size:13px'>Sin datos para este periodo</div>"
"    </div>"
"    <div id='lt-stats' class='lpanel card'>"
"      <div style='font-size:15px;font-weight:600;margin-bottom:14px'>Estadisticas de produccion</div>"
"      <div id='lstats-grid' class='lsg'></div>"
"      <div style='display:grid;grid-template-columns:1fr 1fr;gap:14px;margin-top:14px'>"
"        <div><div style='font-size:11px;color:var(--mut);text-transform:uppercase;margin-bottom:8px'>Metros por dia</div><canvas class='lcanv' id='lst-daily' height='160'></canvas></div>"
"        <div><div style='font-size:11px;color:var(--mut);text-transform:uppercase;margin-bottom:8px'>Sesiones por motivo</div><canvas class='lcanv' id='lst-reason' height='160'></canvas></div>"
"      </div>"
"    </div>"
"  </div>" /* /logs */

"  </div>" /* /content */
"</div>" /* /main */
/* Fullscreen modal */
"<div class='fmo' id='fmo' onclick='if(event.target===this)closeMax()'>"
"  <div class='fmc'>"
"    <div class='fmh'><h3 id='fmtitle'>---</h3><button class='btn bgh' onclick='closeMax()'>&#10005; Cerrar</button></div>"
"    <div class='fmb' id='fmb'></div>"
"  </div>"
"</div>"
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
"  if(id==='logs')loadAllLogs();"
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
"function setPb(id,pct){"
"  const el=document.getElementById(id);"
"  if(el)el.style.width=Math.min(100,Math.max(0,pct))+'%';"
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
"  const avgSpd=parseFloat(d.avg_speed)||0;"
"  document.getElementById('avgsp').textContent=avgSpd.toFixed(2);"
"  const totm=parseFloat(d.total_m)||0;"
"  document.getElementById('totm').textContent=totm.toFixed(2);"
"  const cuts=parseInt(d.cuts)||0;"
"  document.getElementById('cuts').textContent=cuts;"
"  const tc=parseInt(d.target_count)||0;"
"  document.getElementById('cutssub').textContent=tc>0?'Objetivo: '+cuts+'/'+tc:'Sin objetivo';"
"  const cd=parseFloat(d.cut_distance_m)||0;"
"  if(cd>0){"
"    document.getElementById('cdist').textContent=cd.toFixed(0);"
"    const pct=((totm%cd)/cd)*100;"
"    setPb('cprog',pct);"
"  }else{document.getElementById('cdist').textContent='---';}"
"  tspd=parseFloat(d.belt_speed)||0;"
"  if(tspd>0)document.getElementById('spdrng').textContent='Objetivo: '+tspd.toFixed(1)+' m/min';"
/* Progress bars */
"  const spdRange=Math.max(tspd>0?tspd*1.5:20,spd+1);"
"  setPb('spdpb',spd/spdRange*100);"
"  setPb('totmpb',cd>0?((totm%cd)/cd)*100:Math.min(100,(totm/100)*100));"
"  setPb('cutspb',tc>0?(cuts/tc)*100:0);"
"  setPb('avgpb',avgSpd/(Math.max(tspd>0?tspd*1.5:20,avgSpd+0.1))*100);"
"  const rec=d.recording,alm=d.alarm_active;"
"  const st=document.getElementById('sysst');"
"  if(alm){st.textContent='ALARMA';st.className='cst cr';}"
"  else if(rec){st.textContent='Grabando';st.className='cst cg';}"
"  else{st.textContent='En espera';st.className='cst cm';}"
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
/* Grid */
"  cx.strokeStyle='rgba(0,230,118,.08)';cx.lineWidth=1;"
"  for(let i=0;i<=4;i++){"
"    const v=mn+(mx-mn)*(i/4);const y=yp(v);"
"    cx.beginPath();cx.moveTo(p.l,y);cx.lineTo(p.l+cw,y);cx.stroke();"
"    cx.fillStyle='#8b949e';cx.font='10px sans-serif';cx.textAlign='right';"
"    cx.fillText(v.toFixed(1),p.l-3,y+4);"
"  }"
/* Target line */
"  if(tspd>0){"
"    const ty=yp(tspd);"
"    cx.strokeStyle='#f78166';cx.lineWidth=1.5;cx.setLineDash([5,4]);"
"    cx.beginPath();cx.moveTo(p.l,ty);cx.lineTo(p.l+cw,ty);cx.stroke();"
"    cx.setLineDash([]);"
"  }"
/* Gradient fill */
"  cx.beginPath();cx.moveTo(xp(0),yp(hist[0]));"
"  for(let i=1;i<hist.length;i++)cx.lineTo(xp(i),yp(hist[i]));"
"  cx.lineTo(xp(hist.length-1),p.t+ch);cx.lineTo(xp(0),p.t+ch);cx.closePath();"
"  const g=cx.createLinearGradient(0,p.t,0,p.t+ch);"
"  g.addColorStop(0,'rgba(0,230,118,.3)');g.addColorStop(1,'rgba(0,230,118,0)');"
"  cx.fillStyle=g;cx.fill();"
/* Line */
"  cx.beginPath();cx.strokeStyle='#00e676';cx.lineWidth=2;"
"  cx.moveTo(xp(0),yp(hist[0]));"
"  for(let i=1;i<hist.length;i++)cx.lineTo(xp(i),yp(hist[i]));"
"  cx.stroke();"
/* Dots at data points (every 15 samples to avoid clutter) */
"  cx.fillStyle='#00e676';"
"  for(let i=0;i<hist.length;i+=15){"
"    cx.beginPath();cx.arc(xp(i),yp(hist[i]),3,0,Math.PI*2);cx.fill();"
"  }"
/* Last point always shown */
"  cx.beginPath();cx.arc(xp(hist.length-1),yp(hist[hist.length-1]),4,0,Math.PI*2);cx.fill();"
/* Time axis */
"  const now=new Date();"
"  cx.fillStyle='#8b949e';cx.font='10px sans-serif';cx.textAlign='center';"
"  for(let i=0;i<=4;i++){"
"    const idx=Math.floor(i*(hist.length-1)/4);"
"    const sec=(hist.length-1-idx)*2;"
"    const t=new Date(now-sec*1000);"
"    cx.fillText(t.getHours().toString().padStart(2,'0')+':'+t.getMinutes().toString().padStart(2,'0'),xp(idx),H-6);"
"  }"
"}"

/* --- MAXIMIZE MODAL --- */
"function maxCard(t){"
"  const mo=document.getElementById('fmo'),mb=document.getElementById('fmb');"
"  const titles={status:'Estado del sistema',profile:'Perfil activo',ip:'IP local',spd:'Velocidad actual',totm:'Metros producidos',cuts:'Cortes realizados',cdist:'Longitud de corte',avgsp:'Vel. promedio sesion',chart:'Velocidad en tiempo real',io:'Entradas / Salidas'};"
"  document.getElementById('fmtitle').textContent=titles[t]||t;"
"  if(t==='chart'){"
"    mb.style.cssText='flex:1;overflow:hidden;padding:18px;display:flex;flex-direction:column';"
"    mb.innerHTML='<canvas id=\"fmChart\" style=\"display:block;width:100%;flex:1;min-height:300px\"></canvas>';"
"    setTimeout(drawFmChart,60);"
"  }else if(t==='io'){"
"    const alm=document.getElementById('io-al').textContent==='ON';"
"    mb.style.cssText='flex:1;overflow:auto;padding:24px;align-items:flex-start;justify-content:flex-start';"
"    mb.innerHTML='<div style=\"width:100%;max-width:600px\"><div style=\"font-size:12px;color:var(--mut);text-transform:uppercase;margin-bottom:14px\">Salidas digitales</div>'+'<div class=\"io\" style=\"font-size:16px;padding:14px 0\"><span>DO1 &ndash; Alarma</span><span class=\"bdg '+(alm?'bal':'bof')+'\">'+(alm?'ON':'OFF')+'</span></div>'+'<div class=\"io\" style=\"font-size:16px;padding:14px 0\"><span>DO2 &ndash; Corte (relay)</span><span class=\"bdg bof\">OFF</span></div></div>';"
"  }else{"
"    mb.style.cssText='flex:1;overflow:auto;padding:24px;display:flex;flex-direction:column;align-items:center;justify-content:center';"
"    const vals={status:[document.getElementById('sysst').textContent,'Estado actual'],profile:[document.getElementById('aprof').textContent,document.getElementById('aprofsub').textContent],ip:[document.getElementById('ipdisp').textContent,document.getElementById('ssiddisp').textContent],spd:[document.getElementById('spd').textContent,'m/min'],totm:[document.getElementById('totm').textContent,'metros producidos'],cuts:[document.getElementById('cuts').textContent,'cortes realizados'],cdist:[document.getElementById('cdist').textContent,'m por corte'],avgsp:[document.getElementById('avgsp').textContent,'m/min promedio']};"
"    const v=vals[t]||['---',''];"
"    mb.innerHTML='<div class=\"fmv\">'+v[0]+'</div><div class=\"fmu\">'+v[1]+'</div>';"
"  }"
"  mo.classList.add('open');"
"}"
"function closeMax(){document.getElementById('fmo').classList.remove('open');}"
"function drawFmChart(){"
"  const cv=document.getElementById('fmChart');if(!cv)return;"
"  const W=cv.offsetWidth,H=Math.max(cv.offsetHeight,350);"
"  cv.width=W;cv.height=H;"
"  const cx=cv.getContext('2d'),p={t:8,r:8,b:24,l:42};"
"  const cw=W-p.l-p.r,ch=H-p.t-p.b;"
"  cx.clearRect(0,0,W,H);"
"  if(hist.length<2){cx.fillStyle='#8b949e';cx.font='14px sans-serif';cx.textAlign='center';cx.fillText('Esperando datos...',W/2,H/2);return;}"
"  const mx=Math.max(...hist,tspd*1.2,0.5)*1.1,mn=0;"
"  const xp=i=>p.l+(i/(hist.length-1))*cw,yp=v=>p.t+ch-((v-mn)/(mx-mn))*ch;"
"  cx.strokeStyle='rgba(0,230,118,.08)';cx.lineWidth=1;"
"  for(let i=0;i<=4;i++){const vv=mn+(mx-mn)*(i/4),y=yp(vv);cx.beginPath();cx.moveTo(p.l,y);cx.lineTo(p.l+cw,y);cx.stroke();cx.fillStyle='#8b949e';cx.font='11px sans-serif';cx.textAlign='right';cx.fillText(vv.toFixed(1),p.l-3,y+4);}"
"  if(tspd>0){const ty=yp(tspd);cx.strokeStyle='#f78166';cx.lineWidth=1.5;cx.setLineDash([5,4]);cx.beginPath();cx.moveTo(p.l,ty);cx.lineTo(p.l+cw,ty);cx.stroke();cx.setLineDash([]);}"
"  cx.beginPath();cx.moveTo(xp(0),yp(hist[0]));for(let i=1;i<hist.length;i++)cx.lineTo(xp(i),yp(hist[i]));cx.lineTo(xp(hist.length-1),p.t+ch);cx.lineTo(xp(0),p.t+ch);cx.closePath();"
"  const g=cx.createLinearGradient(0,p.t,0,p.t+ch);g.addColorStop(0,'rgba(0,230,118,.3)');g.addColorStop(1,'rgba(0,230,118,0)');cx.fillStyle=g;cx.fill();"
"  cx.beginPath();cx.strokeStyle='#00e676';cx.lineWidth=2;cx.moveTo(xp(0),yp(hist[0]));for(let i=1;i<hist.length;i++)cx.lineTo(xp(i),yp(hist[i]));cx.stroke();"
"  cx.fillStyle='#00e676';for(let i=0;i<hist.length;i+=15){cx.beginPath();cx.arc(xp(i),yp(hist[i]),3,0,Math.PI*2);cx.fill();}"
"  cx.beginPath();cx.arc(xp(hist.length-1),yp(hist[hist.length-1]),5,0,Math.PI*2);cx.fill();"
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
"let allLogs=[];"
"function ltab(el){"
"  document.querySelectorAll('.ltab').forEach(t=>t.classList.remove('act'));"
"  el.classList.add('act');"
"  const t=el.dataset.lt;"
"  document.querySelectorAll('.lpanel').forEach(p=>p.classList.remove('act'));"
"  document.getElementById('lt-'+t).classList.add('act');"
"  if(t==='profile')renderProfileTab();"
"  else if(t==='period')renderPeriodTab();"
"  else if(t==='stats')renderStatsTab();"
"}"
"function parseLogLine(l){"
"  const p=l.split(';');if(p.length<7)return null;"
"  return{start:p[0].trim(),end:p[1].trim(),profile:p[2].trim(),meters:parseFloat(p[3])||0,avgSpeed:parseFloat(p[4])||0,cuts:parseInt(p[5])||0,reason:p[6].trim()};"
"}"
"function parseDateStr(s){"
"  const[d,t]=s.split(' ');if(!d)return null;"
"  const[dd,mm,yy]=d.split('-');const[hh,mi,ss]=(t||'00:00:00').split(':');"
"  return new Date(yy,mm-1,dd,hh||0,mi||0,ss||0);"
"}"
"function sessionMin(row){"
"  const s=parseDateStr(row.start),e=parseDateStr(row.end);if(!s||!e)return 0;"
"  return Math.max(0,(e-s)/60000);"
"}"
"async function loadAllLogs(){"
"  try{"
"    const r=await fetch('/api/logs/all');"
"    if(!r.ok){allLogs=[];}"
"    else{const txt=await r.text();allLogs=txt.trim().split('\\n').map(parseLogLine).filter(x=>x!==null);}"
"  }catch(e){allLogs=[];}"
"  const act=document.querySelector('.ltab.act');"
"  const t=act?act.dataset.lt:'profile';"
"  if(t==='profile')renderProfileTab();"
"  else if(t==='period')renderPeriodTab();"
"  else if(t==='stats')renderStatsTab();"
"}"
"function lstat(v,l,u){return '<div class=\"lst\"><div class=\"lsv\">'+v+'</div><div class=\"lsl\">'+l+(u?' <span style=\"font-size:10px\">'+u+'</span>':'')+'</div></div>';}"
"function renderProfileTab(){"
"  const sel=document.getElementById('lp-sel');"
"  const profs=[...new Set(allLogs.map(r=>r.profile))].sort();"
"  const cur=sel.value;"
"  sel.innerHTML='<option value=\"\">Todos los perfiles</option>';"
"  profs.forEach(p=>sel.innerHTML+='<option value=\"'+p+'\"'+( cur===p?' selected':'')+'>'+p+'</option>');"
"  const filter=sel.value;"
"  const data=filter?allLogs.filter(r=>r.profile===filter):allLogs;"
"  const em=document.getElementById('lp-empty');"
"  if(!data.length){em.style.display='';document.getElementById('lp-stats').innerHTML='';return;}"
"  em.style.display='none';"
"  const totM=data.reduce((a,r)=>a+r.meters,0);"
"  const totC=data.reduce((a,r)=>a+r.cuts,0);"
"  document.getElementById('lp-stats').innerHTML=lstat(totM.toFixed(1),'Metros totales','m')+lstat(totC,'Cortes','total')+lstat(data.length,'Sesiones','');"
"  if(filter){"
"    drawBarChart(document.getElementById('lp-chart'),data.map((_,i)=>'S'+(i+1)),data.map(r=>r.meters),'m');"
"  }else{"
"    const pm={};allLogs.forEach(r=>{pm[r.profile]=(pm[r.profile]||0)+r.meters;});"
"    const keys=Object.keys(pm).sort();"
"    drawBarChart(document.getElementById('lp-chart'),keys,keys.map(k=>pm[k]),'m');"
"  }"
"}"
"function renderPeriodTab(){"
"  const period=document.getElementById('period-sel').value;"
"  const now=new Date();"
"  let data=allLogs;"
"  if(period==='day'){data=allLogs.filter(r=>{const d=parseDateStr(r.start);return d&&d.toDateString()===now.toDateString();});}"
"  else if(period==='week'){const s=new Date(now);s.setDate(now.getDate()-now.getDay());s.setHours(0,0,0,0);data=allLogs.filter(r=>{const d=parseDateStr(r.start);return d&&d>=s;});}"
"  else if(period==='month'){data=allLogs.filter(r=>{const d=parseDateStr(r.start);return d&&d.getMonth()===now.getMonth()&&d.getFullYear()===now.getFullYear();});}"
"  else if(period==='year'){data=allLogs.filter(r=>{const d=parseDateStr(r.start);return d&&d.getFullYear()===now.getFullYear();});}"
"  const em=document.getElementById('lper-empty');"
"  if(!data.length){em.style.display='';document.getElementById('lper-stats').innerHTML='';return;}"
"  em.style.display='none';"
"  const totM=data.reduce((a,r)=>a+r.meters,0);"
"  const totC=data.reduce((a,r)=>a+r.cuts,0);"
"  document.getElementById('lper-stats').innerHTML=lstat(totM.toFixed(1),'Metros','m')+lstat(totC,'Cortes','total')+lstat(data.length,'Sesiones','');"
"  const grp={};const DN=['Dom','Lun','Mar','Mie','Jue','Vie','Sab'],MN=['Ene','Feb','Mar','Abr','May','Jun','Jul','Ago','Sep','Oct','Nov','Dic'];"
"  data.forEach(r=>{const d=parseDateStr(r.start);if(!d)return;let k;"
"    if(period==='day')k=d.getHours().toString().padStart(2,'0')+'h';"
"    else if(period==='week')k=DN[d.getDay()];"
"    else if(period==='month')k=d.getDate().toString().padStart(2,'0');"
"    else k=MN[d.getMonth()];"
"    grp[k]=(grp[k]||0)+r.meters;});"
"  const keys=Object.keys(grp);"
"  drawBarChart(document.getElementById('lper-chart'),keys,keys.map(k=>grp[k]),'m');"
"}"
"function renderStatsTab(){"
"  if(!allLogs.length){document.getElementById('lstats-grid').innerHTML=lstat('---','Sin datos','');return;}"
"  const totM=allLogs.reduce((a,r)=>a+r.meters,0);"
"  const totC=allLogs.reduce((a,r)=>a+r.cuts,0);"
"  const n=allLogs.length;"
"  const avgSp=allLogs.reduce((a,r)=>a+r.avgSpeed,0)/n;"
"  const prodMin=allLogs.reduce((a,r)=>a+sessionMin(r),0);"
"  const manual=allLogs.filter(r=>r.reason==='manual').length;"
"  document.getElementById('lstats-grid').innerHTML="
"    lstat(totM.toFixed(1),'Metros extruidos','m')+lstat(totC,'Cortes realizados','')+lstat(n,'Sesiones totales','')"
"    +lstat(avgSp.toFixed(2),'Vel. promedio','m/min')+lstat((prodMin/60).toFixed(1),'Horas productivas','h')+lstat(manual+'/'+n,'Paros manuales','');"
"  const days={};allLogs.forEach(r=>{const d=parseDateStr(r.start);if(!d)return;const k=d.getDate().toString().padStart(2,'0')+'/'+( d.getMonth()+1).toString().padStart(2,'0');days[k]=(days[k]||0)+r.meters;});"
"  const dk=Object.keys(days);"
"  drawBarChart(document.getElementById('lst-daily'),dk,dk.map(k=>days[k]),'m');"
"  drawBarChart(document.getElementById('lst-reason'),['Manual','Completado'],[manual,n-manual],'');"
"}"
"function drawBarChart(cv,labels,values){"
"  if(!cv||!labels.length)return;"
"  cv.width=cv.offsetWidth||cv.parentElement.offsetWidth||600;"
"  const W=cv.width,H=parseInt(cv.height)||200;"
"  const cx=cv.getContext('2d'),p={t:10,r:10,b:36,l:44};"
"  const cw=W-p.l-p.r,ch=H-p.t-p.b;"
"  cx.clearRect(0,0,W,H);"
"  const mx2=Math.max(...values,0.1);"
"  cx.strokeStyle='rgba(0,230,118,.06)';cx.lineWidth=1;"
"  for(let i=0;i<=4;i++){const y=p.t+ch*(1-i/4);cx.beginPath();cx.moveTo(p.l,y);cx.lineTo(p.l+cw,y);cx.stroke();cx.fillStyle='#8b949e';cx.font='10px sans-serif';cx.textAlign='right';cx.fillText((mx2*i/4).toFixed(mx2<10?1:0),p.l-3,y+3);}"
"  const bw=Math.max(4,(cw/labels.length)*0.65);"
"  values.forEach((v,i)=>{"
"    const x=p.l+(i+0.5)*(cw/labels.length)-bw/2;"
"    const bh=Math.max(1,(v/mx2)*ch),y=p.t+ch-bh;"
"    const g=cx.createLinearGradient(0,y,0,y+bh);"
"    g.addColorStop(0,'rgba(0,230,118,.9)');g.addColorStop(1,'rgba(0,230,118,.35)');"
"    cx.fillStyle=g;cx.beginPath();"
"    if(typeof cx.roundRect==='function')cx.roundRect(x,y,bw,bh,3);else cx.rect(x,y,bw,bh);"
"    cx.fill();"
"    cx.fillStyle='#8b949e';cx.font='9px sans-serif';cx.textAlign='center';"
"    cx.fillText(labels[i],p.l+(i+0.5)*(cw/labels.length),H-8);"
"  });"
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
// /api/logs/all  — todos los CSV históricos concatenados
// =============================================================
static esp_err_t logs_all_handler(httpd_req_t *req)
{
    DIR *dir = opendir("/sdcard/logs");
    if (!dir)
        return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Logs dir not found");

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");

    struct dirent *entry;
    char filepath[512];
    bool sent_any = false;

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "production_", 11) != 0) continue;

        snprintf(filepath, sizeof(filepath), "/sdcard/logs/%s", entry->d_name);
        FILE *f = fopen(filepath, "r");
        if (!f) continue;

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        rewind(f);

        if (size > 0) {
            char *buf = malloc(size + 2);
            if (buf) {
                size_t n = fread(buf, 1, size, f);
                buf[n] = 0;
                while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r'))
                    buf[--n] = 0;
                if (n > 0) {
                    if (sent_any) httpd_resp_sendstr_chunk(req, "\n");
                    httpd_resp_sendstr_chunk(req, buf);
                    sent_any = true;
                }
                free(buf);
            }
        }
        fclose(f);
    }
    closedir(dir);

    httpd_resp_sendstr_chunk(req, NULL);
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
    config.max_uri_handlers = 12;
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
        { .uri = "/api/logs/all", .method = HTTP_GET,    .handler = logs_all_handler     },
    };

    for (int i = 0; i < (int)(sizeof(routes) / sizeof(routes[0])); i++)
        httpd_register_uri_handler(server, &routes[i]);

    ESP_LOGI(TAG, "HTTP server started");
}
