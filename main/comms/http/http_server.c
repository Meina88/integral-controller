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
#include "logic/alarm.h"
#include "logic/alarm_config.h"
#include "logic/profile.h"
#include "logic/active_profile.h"
#include "comms/wifi/wifi_manager.h"
#include "comms/ota/ota_manager.h"

static const char *TAG = "http";

static const char *ota_state_to_str(ota_state_t state)
{
    switch (state) {
    case OTA_STATE_IDLE:        return "idle";
    case OTA_STATE_CHECKING:    return "checking";
    case OTA_STATE_DOWNLOADING: return "downloading";
    case OTA_STATE_SUCCESS:     return "success";
    case OTA_STATE_ERROR:       return "error";
    default:                    return "unknown";
    }
}

// =============================================================
// HTML  (3 chunks sent via httpd_resp_sendstr_chunk)
// =============================================================

static const char HTML_HEAD[] =
"<!DOCTYPE html>"
"<html lang='es'><head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>Rubber SRL CLIENT</title>"
"<style>"
":root{--bg:#0d1117;--sb:#010409;--card:#161b22;--card2:#1c2128;--brd:#30363d;"
"--txt:#e6edf3;--mut:#8b949e;--grn:#00e676;--grn2:rgba(0,230,118,.08);--grn3:rgba(0,230,118,.18);"
"--blu:#58a6ff;--org:#f78166;--yel:#d29922;--red:#f85149}"
"[data-theme=light]{--bg:#f5f5f5;--sb:#ffffff;--card:#ffffff;--card2:#f8f9fa;--brd:#e0e0e0;"
"--txt:#212121;--mut:#757575;--grn:#00897b;--grn2:rgba(0,137,123,.08);--grn3:rgba(0,137,123,.18);"
"--blu:#1976d2;--org:#e64a19;--yel:#f57c00;--red:#c62828}"
"[data-theme=light] .sb{box-shadow:inset 0 -120px 120px rgba(0,137,123,.05)}"
"[data-theme=light] .pf{background:var(--grn);box-shadow:0 0 6px rgba(0,137,123,.4)}"
"[data-theme=light] .bp{background:var(--grn)}"
"[data-theme=light] .bp:hover{box-shadow:0 0 12px rgba(0,137,123,.4)}"
"*{box-sizing:border-box;margin:0;padding:0}"
"body{background:var(--bg);color:var(--txt);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;display:flex;height:100vh;height:100dvh;overflow:hidden}"
".sb{width:clamp(150px,13vw,210px);background:var(--sb);border-right:1px solid var(--brd);display:flex;flex-direction:column;flex-shrink:0;box-shadow:inset 0 -120px 120px rgba(0,230,118,.08)}"
".logo{padding:clamp(10px,1.2vh,18px) clamp(10px,1.2vw,18px);border-bottom:1px solid var(--brd)}"
".logo h1{font-size:clamp(12px,1.1vw,18px);font-weight:700;color:var(--grn)}.logo p{font-size:clamp(9px,0.8vw,12px);color:var(--mut);margin-top:2px}"
"nav{flex:1;padding:6px 0;overflow-y:auto}"
".ns{font-size:clamp(9px,0.75vw,11px);color:var(--grn);padding:clamp(8px,1vh,12px) 16px 4px;text-transform:uppercase;letter-spacing:.1em;font-weight:700}"
".ni{display:flex;align-items:center;gap:8px;padding:clamp(8px,1vh,12px) 16px;cursor:pointer;font-size:clamp(12px,1.1vw,16px);color:var(--mut);border-left:3px solid transparent;transition:all .15s;user-select:none}"
".ni:hover{color:var(--txt);background:var(--grn2)}"
".ni.act{color:var(--grn);background:var(--grn2);border-left-color:var(--grn);text-shadow:0 0 8px rgba(0,230,118,.4)}"
".main{flex:1;display:flex;flex-direction:column;overflow:hidden;min-width:0}"
".topbar{padding:clamp(8px,1vh,14px) clamp(12px,1.5vw,22px);border-bottom:1px solid var(--brd);display:flex;align-items:center;justify-content:space-between;background:var(--sb);flex-shrink:0}"
".topbar h2{font-size:clamp(14px,1.5vw,26px);font-weight:700}.topbar p{font-size:clamp(10px,0.9vw,14px);color:var(--mut);margin-top:1px}"
".tbr{display:flex;align-items:center;gap:clamp(8px,1.2vw,20px)}"
".wbg{display:flex;align-items:center;gap:6px;font-size:clamp(11px,1vw,15px)}"
".dot{width:8px;height:8px;border-radius:50%;display:inline-block;flex-shrink:0}"
".dg{background:var(--grn);box-shadow:0 0 8px var(--grn)}.dr{background:var(--red)}"
".content{flex:1;overflow-y:auto;padding:clamp(8px,0.8vw,14px);display:flex;flex-direction:column;min-height:0}"
".content.dash-mode{overflow:hidden}"
".grid{display:grid;gap:clamp(6px,0.7vw,11px);margin-bottom:clamp(6px,0.7vw,11px)}"
".g4{grid-template-columns:repeat(4,1fr)}"
".g5{grid-template-columns:repeat(5,1fr)}"
".g6{grid-template-columns:repeat(6,1fr)}"
".g7{grid-template-columns:repeat(7,1fr)}"
".g21{grid-template-columns:2fr 1fr}.g211{grid-template-columns:2fr 1fr 1fr}"
".topbar-status{display:flex;align-items:center;gap:clamp(14px,2vw,36px);flex:1;justify-content:center;overflow:hidden}"
".topbar-status .cst{font-size:clamp(11px,0.9vw,14px)!important;font-weight:700}"
".tsitem{display:flex;align-items:baseline;gap:4px;white-space:nowrap}"
".tslbl{font-size:clamp(9px,0.75vw,11px);color:var(--mut);text-transform:uppercase;letter-spacing:.04em}"
".g12{grid-template-columns:1fr 1fr}"
".g3{grid-template-columns:repeat(3,1fr)}"
".card{background:var(--card);border:1px solid var(--brd);border-radius:8px;padding:clamp(10px,1vw,16px)}"
".ch{display:flex;align-items:flex-start;justify-content:space-between;margin-bottom:clamp(4px,0.5vh,8px)}"
".ct{font-size:clamp(10px,0.9vw,14px);color:var(--mut);text-transform:uppercase;letter-spacing:.05em}"
".ci{width:34px;height:34px;border-radius:50%;background:var(--grn2);border:1px solid var(--grn3);display:flex;align-items:center;justify-content:center;font-size:15px;color:var(--grn);flex-shrink:0}"
".cv{font-size:clamp(28px,3.6vw,64px);font-weight:700;color:var(--grn)}"
".cu{font-size:clamp(12px,1.1vw,18px);font-weight:400;color:var(--mut);margin-left:4px}"
".cs{font-size:clamp(10px,0.9vw,14px);color:var(--mut);margin-top:clamp(2px,0.4vh,6px)}"
".cst{font-size:clamp(18px,2.2vw,40px);font-weight:600}"
".cg{color:var(--grn)}.cb{color:var(--blu)}.co{color:var(--org)}.cy{color:var(--yel)}.cr{color:var(--red)}.cm{color:var(--mut)}"
".pb{height:clamp(3px,0.4vh,6px);background:rgba(0,230,118,.12);border-radius:2px;margin-top:clamp(8px,1vh,12px);overflow:hidden}"
".pf{height:100%;border-radius:2px;transition:width .6s ease;background:var(--grn);box-shadow:0 0 6px rgba(0,230,118,.5)}"
".io{display:flex;align-items:center;justify-content:space-between;padding:clamp(5px,0.7vh,9px) 0;border-bottom:1px solid var(--brd);font-size:clamp(11px,1vw,15px)}"
".io:last-child{border-bottom:none}"
".bdg{font-size:clamp(10px,0.9vw,14px);padding:clamp(2px,0.3vh,4px) clamp(6px,0.7vw,12px);border-radius:12px;font-weight:600}"
".bon{background:rgba(0,230,118,.15);color:var(--grn)}"
".bof{background:rgba(139,148,158,.12);color:var(--mut)}"
".bal{background:rgba(248,81,73,.15);color:var(--red)}"
".ali{padding:clamp(6px,0.8vw,12px);background:rgba(248,81,73,.08);border:1px solid rgba(248,81,73,.2);border-radius:6px;margin-bottom:8px;font-size:clamp(11px,0.9vw,14px)}"
".alt{font-weight:600;color:var(--org);margin-bottom:2px}"
".sm{text-align:center}.sm .sv{font-size:clamp(16px,2vw,32px);font-weight:700;color:var(--grn)}.sm .sl{font-size:clamp(10px,0.85vw,13px);color:var(--mut)}"
".view{display:none}.view.act{display:block}"
"#v-dashboard.act{display:flex;flex-direction:column;gap:clamp(6px,0.7vw,11px);flex:1;min-height:0}"
"#v-dashboard .grid{margin-bottom:0}"
"#v-dashboard .g21{flex:1;min-height:0}"
"#v-dashboard .g21>.card{overflow:hidden}"
"#v-dashboard .chart-full{flex:1;min-height:0;overflow:hidden}"
"#v-dashboard .io-row{flex-shrink:0}"
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
".pimg{max-width:100%;max-height:180px;object-fit:contain;border-radius:6px;border:1px solid var(--brd);background:var(--card2);display:block;margin:0 auto 8px}"
".fup{display:flex;align-items:center;gap:8px;flex-wrap:wrap;margin-top:4px}"
"input[type=file]{font-size:12px;color:var(--mut);flex:1;min-width:0}"
".tabs{display:flex;border-bottom:1px solid var(--brd);margin-bottom:10px}"
".tab{padding:7px 14px;cursor:pointer;font-size:12px;color:var(--mut);border-bottom:2px solid transparent}"
".tab.act{color:var(--grn);border-bottom-color:var(--grn)}"
"table{width:100%;border-collapse:collapse;font-size:12px}"
"th{background:var(--card2);color:var(--grn);padding:8px 10px;text-align:left;font-size:10px;text-transform:uppercase;letter-spacing:.06em}"
"td{padding:8px 10px;border-bottom:1px solid var(--brd)}"
"tr:hover td{background:var(--grn2)}"
"#speedChart{display:block;width:100%}"
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
".mob-nav{display:none;position:fixed;bottom:0;left:0;right:0;height:56px;background:var(--sb);border-top:1px solid var(--brd);z-index:100;align-items:center;justify-content:space-around}"
".mob-ni{display:flex;flex-direction:column;align-items:center;font-size:11px;color:var(--mut);cursor:pointer;padding:6px 20px;user-select:none}"
".mob-ni.act{color:var(--grn)}"
".mob-ni-ico{font-size:22px;margin-bottom:2px}"
"@media(max-width:1400px){.g6{grid-template-columns:repeat(3,1fr)}.g7{grid-template-columns:repeat(4,1fr)}}"
"@media(max-width:1300px){.g5{grid-template-columns:repeat(3,1fr)}.g4{grid-template-columns:repeat(2,1fr)}.g3{grid-template-columns:repeat(2,1fr)}.topbar-status{gap:clamp(10px,1.5vw,24px)}}"
"@media(max-width:900px){.g5{grid-template-columns:repeat(2,1fr)}.g6{grid-template-columns:repeat(2,1fr)}.g7{grid-template-columns:repeat(3,1fr)}.g21{grid-template-columns:1fr}.lsg{grid-template-columns:repeat(2,1fr)}.topbar-status{display:none}}"
"@media(max-width:640px){.sb{display:none}.mob-nav{display:flex}.content{padding-bottom:60px}"
"#v-dashboard .g21,#v-dashboard .chart-full,#v-dashboard .io-row{display:none}"
".g5{grid-template-columns:1fr 1fr}.g3{grid-template-columns:1fr 1fr}"
".cv{font-size:clamp(32px,10vw,54px)}.cst{font-size:clamp(22px,7vw,40px)}.sm .sv{font-size:clamp(18px,5vw,28px)}}"
"</style></head>";

// ------------------------------------

static const char HTML_BODY[] =
"<body data-theme='dark'>"
"<div class='sb'>"
"  <div class='logo'><h1>&#9654; Rubber SRL</h1><p>CONTROLLER</p></div>"
"  <nav>"
"    <div class='ns'>Produccion</div>"
"    <div class='ni act' data-view='dashboard' onclick='nav(this)'>&#9632;&nbsp; Dashboard</div>"
"    <div class='ni' data-view='profiles' onclick='nav(this)'>&#9776;&nbsp; Perfiles</div>"
"    <div class='ni' data-view='logs' onclick='nav(this)'>&#9741;&nbsp; Logs</div>"
"    <div class='ns'>Sistema</div>"
"    <div class='ni' data-view='diagnostico' onclick='nav(this)'>&#9881;&nbsp; Diagnostico</div>"
"  </nav>"
"  <div style='padding:10px 16px;font-size:11px;color:var(--mut);border-top:1px solid var(--brd)'>v1.0.0 &copy; 2025 Rubber SRL</div>"
"</div>"
"<div class='main'>"
"  <div class='topbar'>"
"    <div><h2 id='vtitle'>Dashboard</h2><p id='vsub'>Resumen general del sistema</p></div>"
"    <div class='topbar-status'>"
"      <div class='tsitem'><span class='tslbl'>Sistema</span><span id='sysst' class='cst cm'>---</span><span id='syssub' style='display:none'></span></div>"
"      <div class='tsitem'><span class='tslbl'>Perfil</span><span id='aprof' style='font-weight:700;color:var(--grn)'>---</span><span id='aprofsub' style='display:none'></span></div>"
"      <div class='tsitem'><span class='tslbl'>IP</span><span id='ipdisp' style='font-weight:700;color:var(--grn);font-family:monospace'>---</span><span id='ssiddisp' style='display:none'></span></div>"
"    </div>"
"    <div class='tbr'>"
"      <div class='wbg'><div class='dot dr' id='wdot'></div><span id='wlbl'>Sin WiFi</span></div>"
"      <button id='theme-btn' class='mx' onclick='toggleTheme()' title='Cambiar tema' style='font-size:16px;cursor:pointer'>🌙</button>"
"      <div style='font-size:11px;color:var(--mut)' id='uptime'>00:00:00</div>"
"    </div>"
"  </div>"
"  <div class='content dash-mode'>"

/* === DASHBOARD VIEW === */
"  <div id='v-dashboard' class='view act'>"

/* 5 metric cards + compact spray */
"    <div class='grid g6'>"
"      <div class='card'><div class='ch'><div class='ct'>Velocidad actual</div><div class='mx' onclick='maxCard(\"spd\")'>&#x26F6;</div></div><div class='cv'><span id='spd'>0.00</span><span class='cu'>m/min</span></div><div class='cs' id='spdrng' style='color:var(--org)'>&nbsp;</div></div>"
"      <div class='card'><div class='ch'><div class='ct'>Metros producidos</div><div class='mx' onclick='maxCard(\"totm\")'>&#x26F6;</div></div><div class='cv'><span id='totm'>0.00</span><span class='cu'>m</span></div><div class='cs' id='totmsub'>&nbsp;</div><div class='pb'><div class='pf' id='totmpb' style='width:0%'></div></div></div>"
"      <div class='card'><div class='ch'><div class='ct'>Cortes realizados</div><div class='mx' onclick='maxCard(\"cuts\")'>&#x26F6;</div></div><div class='cv'><span id='cuts'>0</span><span class='cu'>cortes</span></div><div class='cs' id='cutssub'>&nbsp;</div><div class='pb'><div class='pf' id='cutspb' style='width:0%'></div></div></div>"
"      <div class='card'><div class='ch'><div class='ct'>Long. de corte</div><div class='mx' onclick='maxCard(\"cdist\")'>&#x26F6;</div></div><div class='cv'><span id='cdist'>---</span><span class='cu'>m</span></div><div class='pb'><div class='pf' id='cprog' style='width:0%'></div></div></div>"
"      <div class='card'><div class='ch'><div class='ct'>Vel. promedio sesion</div><div class='mx' onclick='maxCard(\"avgsp\")'>&#x26F6;</div></div><div class='cv'><span id='avgsp'>0.00</span><span class='cu'>m/min</span></div><div class='cs'>Sesion actual</div></div>"
"      <div class='card' style='display:flex;flex-direction:column'>"
"        <div class='ch'><div class='ct'>Pintura restante</div></div>"
"        <div style='flex:1;display:flex;flex-direction:column;align-items:center;justify-content:center;text-align:center'>"
"          <canvas id='sprayArc' style='width:min(100%,80px);display:block'></canvas>"
"          <div style='margin-top:6px'><span id='sprayRem' style='font-size:clamp(16px,1.5vw,28px);font-weight:700;color:var(--grn)'>---</span><span class='cu' style='font-size:11px'>&nbsp;disp.</span></div>"
"          <div class='cs' style='font-size:10px;margin-top:2px' id='spraySub'>---</div>"
"          <div id='sprayWarn' style='display:none;font-size:10px;font-weight:600;color:var(--red)'>&#9888; Bajo</div>"
"        </div>"
"      </div>"
"    </div>"

/* Full-width chart */
"    <div class='card chart-full'>"
"      <div class='ch'>"
"        <span class='ct'>Velocidad en tiempo real</span>"
"        <div style='display:flex;align-items:center;gap:10px'><span style='font-size:11px;color:var(--mut)'><span style='color:var(--grn)'>&#8212; Actual</span>&nbsp;&nbsp;<span style='color:var(--org)'>- - Objetivo</span></span><div class='mx' onclick='maxCard(\"chart\")'>&#x26F6;</div></div>"
"      </div>"
"      <canvas id='speedChart'></canvas>"
"      <div class='grid g4' style='margin-top:10px;margin-bottom:0'>"
"        <div class='sm'><div class='sv' id='st-avg'>0.00</div><div class='sl'>Promedio</div></div>"
"        <div class='sm'><div class='sv' id='st-max'>--</div><div class='sl'>Maxima</div></div>"
"        <div class='sm'><div class='sv' id='st-min'>--</div><div class='sl'>Minima</div></div>"
"        <div class='sm'><div class='sv' id='st-dev'>0.00</div><div class='sl'>Desv. Est.</div></div>"
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
"        <div id='pcount' style='font-size:11px;color:var(--mut);padding:2px 2px 6px'></div>"
"        <div class='pl' id='plist'></div>"
"      </div>"
"      <div class='ple card'>"
"        <div id='pempty' style='display:flex;align-items:center;justify-content:center;height:200px;color:var(--mut);font-size:13px'>Selecciona un perfil para editar</div>"
"        <div id='pform' style='display:none'>"
"          <div style='display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:8px;margin-bottom:12px'>"
"            <div style='font-size:15px;font-weight:600' id='pfmtitle'>Nuevo perfil</div>"
"            <div class='br' style='margin:0'>"
"              <button class='btn bgh' onclick='cancelEdit()'>Cancelar</button>"
"              <button class='btn bd' id='bdel' onclick='delProf()'>Eliminar</button>"
"              <button class='btn bp' onclick='saveProf()'>Guardar</button>"
"            </div>"
"          </div>"
"          <div style='display:flex;gap:20px;align-items:flex-start'>"
"            <div style='flex:3;min-width:0'>"
"              <div class='fsec'>General</div>"
"              <div class='fg'>"
"                <div class='fgs'><label>Codigo (ID)</label><input id='f_code' placeholder='ej: 1000.5000'/></div>"
"                <div class='fgs'><label>Nombre comercial</label><input id='f_name' placeholder='ej: Manguera 10mm'/></div>"
"              </div>"
"              <div class='fsec'>Geometria</div>"
"              <div class='fg'>"
"                <div class='fgs'><label>Matriz</label><input id='f_matrix' placeholder='5000'/></div>"
"                <div class='fgs'><label>Bocas</label><input id='f_bocas' type='number' min='1' value='1'/></div>"
"                <div class='fgs'><label>Area (mm&sup2;)</label><input id='f_area' type='number' step='0.01' value='0'/></div>"
"              </div>"
"              <div class='fsec'>Proceso</div>"
"              <div class='fg'>"
"                <div class='fgs'><label>Husillo (%)</label><input id='f_screw' type='number' min='0' max='100' value='25'/></div>"
"                <div class='fgs'><label>RPM VFD objetivo</label><input id='f_vfd' type='number' value='450'/></div>"
"                <div class='fgs'><label>Vel. correa obj. (m/min)</label><input id='f_belt' type='number' step='0.1' value='8.0'/></div>"
"              </div>"
"              <div class='fsec'>Produccion</div>"
"              <div class='fg'>"
"                <div class='fgs'><label>Opciones de corte (m, separar con coma)</label><input id='f_cuts' placeholder='50, 100, 200'/></div>"
"                <div class='fgs'><label>Corte por defecto (m)</label><input id='f_defcut' type='number' step='0.5' value='50'/></div>"
"                <div class='fgs'><label>Permitir valor personalizado</label>"
"                  <select id='f_custom'><option value='true'>Si</option><option value='false'>No</option></select>"
"                </div>"
"              </div>"
"              <div class='fsec'>Ingenieria</div>"
"              <div class='fg'>"
"                <div class='fgs'><label>Densidad teorica (gr/m)</label><input id='f_tden' type='number' step='0.01' value='0.20'/></div>"
"                <div class='fgs'><label>Densidad real (gr/m)</label><input id='f_rden' type='number' step='0.01' value='0.20'/></div>"
"              </div>"
"            </div>"
"            <div style='flex:2;min-width:220px;position:sticky;top:0'>"
"              <div class='fsec'>Imagen del perfil</div>"
"              <img id='profimg' class='pimg' alt='' style='display:none'>"
"              <div id='no-img-lbl' style='height:160px;display:flex;align-items:center;justify-content:center;background:var(--card2);border:1px solid var(--brd);border-radius:6px;color:var(--mut);font-size:12px;margin-bottom:8px'>Sin imagen</div>"
"              <div class='fup'>"
"                <input type='file' id='f_img' accept='image/png,image/jpeg'>"
"                <button class='btn bp' onclick='uploadImg()'>Subir</button>"
"                <button class='btn bd' id='btn-del-img' onclick='delImg()' style='display:none'>Eliminar</button>"
"              </div>"
"            </div>"
"          </div>"
"        </div>"
"      </div>"
"    </div>"
"  </div>" /* /profiles */






/* === LOGS VIEW === */
"  <div id='v-logs' class='view'>"
"    <div class='plo'>"
"      <div class='pll'>"
"        <div class='br' style='margin:0 0 10px'>"
"          <button class='btn bp' style='flex:1' onclick='loadLogList()'>&#8635; Actualizar</button>"
"        </div>"
"        <div id='log-list-count' style='font-size:11px;color:var(--mut);padding:2px 2px 6px'></div>"
"        <div class='pl' id='log-list'></div>"
"      </div>"
"      <div class='ple card' id='log-viewer'>"
"        <div style='display:flex;align-items:center;justify-content:center;height:200px;color:var(--mut);font-size:13px'>Selecciona un archivo de log</div>"
"      </div>"
"    </div>"
"  </div>" /* /logs */






/* === DIAGNOSTICO VIEW === */
"  <div id='v-diagnostico' class='view'>"
"    <div class='grid g12' style='height:100%;align-content:start'>"

"      <div class='card'>"
"        <div class='ch'><div class='ct'>Conexion WiFi</div></div>"
"        <div style='display:flex;align-items:center;gap:14px;margin:clamp(8px,1.5vh,20px) 0'>"
"          <div class='dot dr' id='diag-wifi-dot' style='width:16px;height:16px;flex-shrink:0'></div>"
"          <span style='font-size:clamp(16px,1.8vw,26px);font-weight:700' id='diag-wifi-lbl'>Sin WiFi</span>"
"        </div>"
"        <div class='io'><span style='color:var(--mut)'>Red (SSID)</span><span id='diag-wifi-ssid' style='font-family:monospace;font-weight:600;color:var(--grn)'>---</span></div>"
"        <div class='io'><span style='color:var(--mut)'>Direcci&oacute;n IP</span><span id='diag-wifi-ip' style='font-family:monospace;font-weight:600;color:var(--grn)'>---</span></div>"
"      </div>"

"      <div class='card'>"
"        <div class='ch'><div class='ct'>Entradas / Salidas</div></div>"
"        <div class='tabs'>"
"          <div class='tab act' data-tab='out' onclick='iotab(this)'>Salidas</div>"
"          <div class='tab' data-tab='in' onclick='iotab(this)'>Entradas</div>"
"        </div>"
"        <div id='io-out'>"
"          <div class='io'><span>DO1 &ndash; Alarma</span><span class='bdg bof' id='io-al'>OFF</span></div>"
"          <div class='io'><span>DO2 &ndash; Pintura</span><span class='bdg bof' id='io-rl'>OFF</span></div>"
"        </div>"
"        <div id='io-in' style='display:none'>"
"          <div class='io'><span>DI1 &ndash; Sensor de velocidad</span><span class='bdg bof' id='io-di1'>OFF</span></div>"
"        </div>"
"        <div style='margin-top:16px;padding-top:14px;border-top:1px solid var(--brd)'>"
"          <div class='ct' style='margin-bottom:10px'>Prueba de relays</div>"
"          <div style='display:flex;gap:10px;flex-wrap:wrap'>"
"            <button class='btn bgh' id='btn-test-alarm' onclick='testRelay(\"alarm\",this)'>&#9654; Probar alarma</button>"
"            <button class='btn bgh' id='btn-test-paint' onclick='testRelay(\"paint\",this)'>&#9654; Probar pintura</button>"
"          </div>"
"        </div>"
"        <div style='margin-top:16px'>"
"          <div class='ct' style='margin-bottom:8px'>Alarmas activas</div>"
"          <div id='alml'><div style='font-size:12px;color:var(--mut)'>Sin alarmas activas</div></div>"
"        </div>"
"      </div>"

"    </div>"
"  </div>" /* /diagnostico */

"  </div>" /* /content */
"</div>" /* /main */
/* Fullscreen modal */
"<div class='fmo' id='fmo' onclick='if(event.target===this)closeMax()'>"
"  <div class='fmc'>"
"    <div class='fmh'><h3 id='fmtitle'>---</h3><button class='btn bgh' onclick='closeMax()'>&#10005; Cerrar</button></div>"
"    <div class='fmb' id='fmb'></div>"
"  </div>"
"</div>"
"<div class='mob-nav'>"
"  <div class='mob-ni act' data-view='dashboard' onclick='nav(this)'><div class='mob-ni-ico'>&#9632;</div>Dashboard</div>"
"  <div class='mob-ni' data-view='profiles' onclick='nav(this)'><div class='mob-ni-ico'>&#9776;</div>Perfiles</div>"
"  <div class='mob-ni' data-view='logs' onclick='nav(this)'><div class='mob-ni-ico'>&#9741;</div>Logs</div>"
"  <div class='mob-ni' data-view='diagnostico' onclick='nav(this)'><div class='mob-ni-ico'>&#9881;</div>Diagn.</div>"
"</div>"
"</body>";

// ------------------------------------

static const char HTML_JS[] =
"<script>"
/* --- VIEWS --- */
"const VIEWS={'dashboard':'Dashboard|Resumen general del sistema','profiles':'Perfiles|Gestion de perfiles','logs':'Logs de produccion|Historico del dia','diagnostico':'Diagnostico|Estado de conexion y E/S'};"
"function nav(el){"
"  const id=el.dataset.view;"
"  document.querySelectorAll('.ni,.mob-ni').forEach(n=>n.classList.remove('act'));"
"  document.querySelectorAll('[data-view=\"'+id+'\"]').forEach(n=>n.classList.add('act'));"
"  document.querySelectorAll('.view').forEach(v=>v.classList.remove('act'));"
"  document.getElementById('v-'+id).classList.add('act');"
"  const[t,s]=VIEWS[id].split('|');"
"  document.getElementById('vtitle').textContent=t;"
"  document.getElementById('vsub').textContent=s;"
"  const c=document.querySelector('.content');"
"  if(c){if(id==='dashboard')c.classList.add('dash-mode');else c.classList.remove('dash-mode');}"
"  if(id==='profiles')loadProfs();"
"  if(id==='logs')loadLogList();"
"}"

/* --- UPTIME --- */
"let online=false;"
"const t0=Date.now();"
"setInterval(()=>{"
"  if(!online)return;"
"  const s=Math.floor((Date.now()-t0)/1000);"
"  const h=String(Math.floor(s/3600)).padStart(2,'0');"
"  const m=String(Math.floor((s%3600)/60)).padStart(2,'0');"
"  document.getElementById('uptime').textContent=h+':'+m+':'+String(s%60).padStart(2,'0');"
"},1000);"

/* --- REAL-TIME DATA --- */
"let hist=[],hmax=-Infinity,hmin=Infinity,hsum=0,hn=0,tspd=0,sprayPctVal=0;"
"function setOnline(v){"
"  online=v;"
"  if(!v){"
"    document.getElementById('wdot').className='dot dr';"
"    document.getElementById('wlbl').textContent='Sin conexion';"
"    const st=document.getElementById('sysst');if(st){st.textContent='Sin conexion';st.className='cst cr';}"
"    const dd=document.getElementById('diag-wifi-dot');if(dd)dd.className='dot dr';"
"    const dl=document.getElementById('diag-wifi-lbl');if(dl)dl.textContent='Sin conexion';"
"  }"
"}"
"async function poll(){"
"  try{"
"    const r=await fetch('/api/status');"
"    if(!r.ok){setOnline(false);return;}"
"    const d=await r.json();"
"    setOnline(true);"
"    update(d);"
"  }catch(e){setOnline(false);}"
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
"  const dd=document.getElementById('diag-wifi-dot'),dl=document.getElementById('diag-wifi-lbl');"
"  const di=document.getElementById('diag-wifi-ip'),ds=document.getElementById('diag-wifi-ssid');"
"  if(dd)dd.className='dot '+(wok?'dg':'dr');"
"  if(dl)dl.textContent=wok?'Conectado':'Sin WiFi';"
"  if(di)di.textContent=d.ip||'---';"
"  if(ds)ds.textContent=wok?(d.wifi_ssid||'---'):'---';"
"  const ral=document.getElementById('io-al'),rrl=document.getElementById('io-rl');"
"  const ar=d.alarm_relay;ral.textContent=ar?'ON':'OFF';ral.className='bdg '+(ar?'bon':'bof');"
"  const rf=d.relay_fired;rrl.textContent=rf?'ON':'OFF';rrl.className='bdg '+(rf?'bon':'bof');"
"  const di1=document.getElementById('io-di1');if(di1){const so=d.sensor_correa;di1.textContent=so?'ON':'OFF';di1.className='bdg '+(so?'bon':'bof');}"
"  const al=document.getElementById('alml');"
"  if(al){if(alm)al.innerHTML='<div class=\"ali\"><div class=\"alt\">&#9888; Velocidad fuera de rango</div><div style=\"color:var(--mut)\">Actual: '+spd.toFixed(2)+' m/min &nbsp;|&nbsp; Objetivo: '+tspd.toFixed(2)+' m/min</div></div>';else al.innerHTML='<div style=\"font-size:12px;color:var(--mut)\">Sin alarmas activas</div>';}"
"  const sRem=parseInt(d.spray_remaining)||0,sMx=parseInt(d.spray_max)||1;"
"  document.getElementById('sprayRem').textContent=sRem;"
"  document.getElementById('spraySub').textContent='Capacidad: '+sMx+' disparos';"
"  sprayPctVal=sMx>0?sRem/sMx:0;"
"  document.getElementById('sprayRem').style.color=sprayPctVal<=0.2?'var(--red)':'var(--grn)';"
"  const wEl=document.getElementById('sprayWarn');if(wEl)wEl.style.display=sprayPctVal<=0.2?'':'none';"
"  drawSprayArc();"
"  drawChart();"
"}"
"function tc(){"
"  const l=document.body.getAttribute('data-theme')==='light';"
"  return{"
"    grn:l?'#00897b':'#00e676',"
"    grid:l?'rgba(0,137,123,.1)':'rgba(0,230,118,.08)',"
"    fill0:l?'rgba(0,137,123,.22)':'rgba(0,230,118,.3)',"
"    fill1:l?'rgba(0,137,123,0)':'rgba(0,230,118,0)',"
"    org:l?'#e64a19':'#f78166',"
"    mut:l?'#9e9e9e':'#8b949e',"
"    red:l?'#c62828':'#f85149',"
"    arcbg:l?'rgba(0,0,0,.1)':'rgba(139,148,158,.15)'"
"  };"
"}"
"function drawSprayArc(){"
"  const cv=document.getElementById('sprayArc');if(!cv||!cv.offsetWidth)return;"
"  const S=Math.min(cv.offsetWidth,180),dpr=window.devicePixelRatio||1;"
"  cv.width=S*dpr;cv.height=S*dpr;cv.style.width=S+'px';cv.style.height=S+'px';"
"  const cx=S/2,cy=S/2,r=S/2-Math.round(S*0.09);"
"  const ctx=cv.getContext('2d');ctx.scale(dpr,dpr);ctx.clearRect(0,0,S,S);"
"  const th=tc(),pct=Math.max(0,Math.min(1,sprayPctVal)),low=pct<=0.2;"
"  const col=low?th.red:th.grn;"
"  const sa=Math.PI*0.75,sw=Math.PI*1.5;"
"  const lw=Math.round(S*0.085);ctx.lineWidth=lw;ctx.lineCap='round';"
"  ctx.beginPath();ctx.arc(cx,cy,r,sa,sa+sw);ctx.strokeStyle=th.arcbg;ctx.stroke();"
"  if(pct>0.001){"
"    ctx.beginPath();ctx.arc(cx,cy,r,sa,sa+sw*pct);ctx.strokeStyle=col;"
"    if(low){ctx.shadowColor=col;ctx.shadowBlur=8;}ctx.stroke();ctx.shadowBlur=0;"
"  }"
"  ctx.fillStyle=col;ctx.textAlign='center';ctx.textBaseline='middle';"
"  ctx.font='bold '+Math.round(S*0.165)+'px sans-serif';ctx.fillText(Math.round(pct*100)+'%',cx,cy+S*0.04);"
"  ctx.font=Math.round(S*0.08)+'px sans-serif';ctx.fillStyle=th.mut;ctx.fillText('restante',cx,cy+S*0.20);"
"}"
"function stddev(){"
"  if(hist.length<2)return 0;"
"  const mn=hist.reduce((a,b)=>a+b,0)/hist.length;"
"  return Math.sqrt(hist.map(v=>(v-mn)**2).reduce((a,b)=>a+b,0)/hist.length);"
"}"

/* --- CHART --- */
"function drawChart(){"
"  const cv=document.getElementById('speedChart');"
"  if(!cv||!cv.offsetWidth)return;"
"  const W=cv.offsetWidth;"
"  const card=cv.parentElement;"
"  const H=Math.max(80,card&&card.clientHeight>120?card.clientHeight-90:Math.round(window.innerHeight*0.35));"
"  const dpr=window.devicePixelRatio||1;"
"  cv.width=W*dpr;cv.height=H*dpr;cv.style.width=W+'px';cv.style.height=H+'px';"
"  const cx=cv.getContext('2d');cx.scale(dpr,dpr);"
"  const p={t:8,r:8,b:24,l:38};"
"  const cw=W-p.l-p.r,ch=H-p.t-p.b;"
"  cx.clearRect(0,0,W,H);"
"  const th=tc();"
"  if(hist.length<2){cx.fillStyle=th.mut;cx.font='12px sans-serif';cx.textAlign='center';cx.fillText('Esperando datos...',W/2,H/2);return;}"
"  const mx=Math.max(...hist,tspd*1.2,0.5)*1.1,mn=0;"
"  const xp=i=>p.l+(i/(hist.length-1))*cw;"
"  const yp=v=>p.t+ch-((v-mn)/(mx-mn))*ch;"
/* Grid */
"  cx.strokeStyle=th.grid;cx.lineWidth=1;"
"  for(let i=0;i<=4;i++){"
"    const v=mn+(mx-mn)*(i/4);const y=yp(v);"
"    cx.beginPath();cx.moveTo(p.l,y);cx.lineTo(p.l+cw,y);cx.stroke();"
"    cx.fillStyle=th.mut;cx.font='10px sans-serif';cx.textAlign='right';"
"    cx.fillText(v.toFixed(1),p.l-3,y+4);"
"  }"
/* Target line */
"  if(tspd>0){"
"    const ty=yp(tspd);"
"    cx.strokeStyle=th.org;cx.lineWidth=1.5;cx.setLineDash([5,4]);"
"    cx.beginPath();cx.moveTo(p.l,ty);cx.lineTo(p.l+cw,ty);cx.stroke();"
"    cx.setLineDash([]);"
"  }"
/* Gradient fill */
"  cx.beginPath();cx.moveTo(xp(0),yp(hist[0]));"
"  for(let i=1;i<hist.length;i++)cx.lineTo(xp(i),yp(hist[i]));"
"  cx.lineTo(xp(hist.length-1),p.t+ch);cx.lineTo(xp(0),p.t+ch);cx.closePath();"
"  const g=cx.createLinearGradient(0,p.t,0,p.t+ch);"
"  g.addColorStop(0,th.fill0);g.addColorStop(1,th.fill1);"
"  cx.fillStyle=g;cx.fill();"
/* Line */
"  cx.beginPath();cx.strokeStyle=th.grn;cx.lineWidth=2;"
"  cx.moveTo(xp(0),yp(hist[0]));"
"  for(let i=1;i<hist.length;i++)cx.lineTo(xp(i),yp(hist[i]));"
"  cx.stroke();"
/* Dots at data points (every 15 samples to avoid clutter) */
"  cx.fillStyle=th.grn;"
"  for(let i=0;i<hist.length;i+=15){"
"    cx.beginPath();cx.arc(xp(i),yp(hist[i]),3,0,Math.PI*2);cx.fill();"
"  }"
/* Last point always shown */
"  cx.beginPath();cx.arc(xp(hist.length-1),yp(hist[hist.length-1]),4,0,Math.PI*2);cx.fill();"
/* Time axis */
"  const now=new Date();"
"  cx.fillStyle=th.mut;cx.font='10px sans-serif';cx.textAlign='center';"
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
"    mb.innerHTML='<div style=\"width:100%;max-width:600px\"><div style=\"font-size:12px;color:var(--mut);text-transform:uppercase;margin-bottom:14px\">Salidas digitales</div>'+'<div class=\"io\" style=\"font-size:16px;padding:14px 0\"><span>DO1 &ndash; Alarma</span><span class=\"bdg '+(alm?'bal':'bof')+'\">'+(alm?'ON':'OFF')+'</span></div>'+'<div class=\"io\" style=\"font-size:16px;padding:14px 0\"><span>DO2 &ndash; Pintura</span><span class=\"bdg bof\">OFF</span></div></div>';"
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
"  const dpr=window.devicePixelRatio||1;"
"  cv.width=W*dpr;cv.height=H*dpr;cv.style.width=W+'px';cv.style.height=H+'px';"
"  const cx=cv.getContext('2d');cx.scale(dpr,dpr);"
"  const p={t:8,r:8,b:24,l:42};"
"  const cw=W-p.l-p.r,ch=H-p.t-p.b;"
"  cx.clearRect(0,0,W,H);"
"  const th=tc();"
"  if(hist.length<2){cx.fillStyle=th.mut;cx.font='14px sans-serif';cx.textAlign='center';cx.fillText('Esperando datos...',W/2,H/2);return;}"
"  const mx=Math.max(...hist,tspd*1.2,0.5)*1.1,mn=0;"
"  const xp=i=>p.l+(i/(hist.length-1))*cw,yp=v=>p.t+ch-((v-mn)/(mx-mn))*ch;"
"  cx.strokeStyle=th.grid;cx.lineWidth=1;"
"  for(let i=0;i<=4;i++){const vv=mn+(mx-mn)*(i/4),y=yp(vv);cx.beginPath();cx.moveTo(p.l,y);cx.lineTo(p.l+cw,y);cx.stroke();cx.fillStyle=th.mut;cx.font='11px sans-serif';cx.textAlign='right';cx.fillText(vv.toFixed(1),p.l-3,y+4);}"
"  if(tspd>0){const ty=yp(tspd);cx.strokeStyle=th.org;cx.lineWidth=1.5;cx.setLineDash([5,4]);cx.beginPath();cx.moveTo(p.l,ty);cx.lineTo(p.l+cw,ty);cx.stroke();cx.setLineDash([]);}"
"  cx.beginPath();cx.moveTo(xp(0),yp(hist[0]));for(let i=1;i<hist.length;i++)cx.lineTo(xp(i),yp(hist[i]));cx.lineTo(xp(hist.length-1),p.t+ch);cx.lineTo(xp(0),p.t+ch);cx.closePath();"
"  const g=cx.createLinearGradient(0,p.t,0,p.t+ch);g.addColorStop(0,th.fill0);g.addColorStop(1,th.fill1);cx.fillStyle=g;cx.fill();"
"  cx.beginPath();cx.strokeStyle=th.grn;cx.lineWidth=2;cx.moveTo(xp(0),yp(hist[0]));for(let i=1;i<hist.length;i++)cx.lineTo(xp(i),yp(hist[i]));cx.stroke();"
"  cx.fillStyle=th.grn;for(let i=0;i<hist.length;i+=15){cx.beginPath();cx.arc(xp(i),yp(hist[i]),3,0,Math.PI*2);cx.fill();}"
"  cx.beginPath();cx.arc(xp(hist.length-1),yp(hist[hist.length-1]),5,0,Math.PI*2);cx.fill();"
"}"

/* --- RELAY TEST --- */
"async function testRelay(r,btn){"
"  const lbl=btn.textContent;"
"  btn.disabled=true;btn.textContent='...';"
"  try{await fetch('/api/test?relay='+r);}catch(e){}"
"  setTimeout(()=>{btn.disabled=false;btn.textContent=lbl;},r==='alarm'?2500:1200);"
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
"  const cnt=document.getElementById('pcount'),total=allCodes.length,shown=codes.length;"
"  if(cnt){if(shown===total)cnt.textContent=total===1?'1 perfil':total+' perfiles';else cnt.textContent=shown+' de '+total+' perfiles';}"
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
"    loadProfileImage();"
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
"  clearProfileImage();"
"}"
"function cancelEdit(){"
"  curCode=null;isNew=false;"
"  document.getElementById('pempty').style.display='';"
"  document.getElementById('pform').style.display='none';"
"  document.querySelectorAll('.pi').forEach(e=>e.classList.remove('sel'));"
"  clearProfileImage();"
"}"
"function clearProfileImage(){"
"  const img=document.getElementById('profimg'),ph=document.getElementById('no-img-lbl');"
"  img.style.display='none';img.src='';"
"  if(ph)ph.style.display='';"
"  document.getElementById('btn-del-img').style.display='none';"
"  document.getElementById('f_img').value='';"
"}"
"function loadProfileImage(){"
"  if(!curCode){clearProfileImage();return;}"
"  const img=document.getElementById('profimg'),ph=document.getElementById('no-img-lbl');"
"  img.src='/api/profile/image?code='+encodeURIComponent(curCode)+'&_='+Date.now();"
"  img.onload=()=>{img.style.display='block';if(ph)ph.style.display='none';document.getElementById('btn-del-img').style.display='';};"
"  img.onerror=()=>{img.style.display='none';if(ph)ph.style.display='';document.getElementById('btn-del-img').style.display='none';};"
"}"
"async function uploadImg(){"
"  const file=document.getElementById('f_img').files[0];"
"  if(!file){alert('Seleccione una imagen (PNG o JPG)');return;}"
"  if(!curCode){alert('Guarde el perfil primero');return;}"
"  if(file.size>512*1024){alert('Imagen demasiado grande (maximo 512 KB)');return;}"
"  try{"
"    const r=await fetch('/api/profile/image?code='+encodeURIComponent(curCode),{method:'POST',body:file,headers:{'Content-Type':file.type||'image/png'}});"
"    if(r.ok){document.getElementById('f_img').value='';loadProfileImage();}"
"    else alert('Error al subir imagen: '+r.status);"
"  }catch(e){alert('Error: '+e);}"
"}"
"async function delImg(){"
"  if(!curCode||!confirm('Eliminar imagen del perfil '+curCode+'?'))return;"
"  try{"
"    const r=await fetch('/api/profile/image?code='+encodeURIComponent(curCode),{method:'DELETE'});"
"    if(r.ok)clearProfileImage();"
"    else alert('No se pudo eliminar la imagen');"
"  }catch(e){}"
"}"
"function buildJSON(code){"
"  const cuts=document.getElementById('f_cuts').value.split(',').map(s=>parseFloat(s.trim())).filter(v=>!isNaN(v));"
"  return JSON.stringify({"
"    general:{code:code,commercial_name:document.getElementById('f_name').value},"
"    geometry:{matrix:document.getElementById('f_matrix').value,bocas:parseInt(document.getElementById('f_bocas').value),area_mm2:parseFloat(document.getElementById('f_area').value)},"
"    production:{cut_options_m:cuts,default_cut_m:parseFloat(document.getElementById('f_defcut').value),allow_custom:document.getElementById('f_custom').value==='true'},"
"    process:{screw:parseInt(document.getElementById('f_screw').value),target_speed_vfd_rpm:parseInt(document.getElementById('f_vfd').value),target_speed_belt_m_min:parseFloat(document.getElementById('f_belt').value)},"
"    engineering:{theoretical_density_gr_m:parseFloat(document.getElementById('f_tden').value),real_density_gr_m:parseFloat(document.getElementById('f_rden').value)},"
"    files:{image:code+'.png'}"
"  },null,2);"
"}"
"async function saveProf(){"
"  const code=document.getElementById('f_code').value.trim();"
"  if(!code){alert('El codigo es requerido');return;}"
"  try{"
"    const r=await fetch('/api/profile?code='+encodeURIComponent(code),{method:'POST',body:buildJSON(code),headers:{'Content-Type':'application/json'}});"
"    if(r.ok){"
"      curCode=code;isNew=false;"
"      document.getElementById('f_code').readOnly=true;"
"      document.getElementById('pfmtitle').textContent='Editando: '+code;"
"      document.getElementById('bdel').style.display='';"
"      loadProfs();"
"    }"
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
"async function loadLogList(){"
"  try{"
"    const r=await fetch('/api/logs/list');if(!r.ok)return;"
"    const files=await r.json();renderLogList(files);"
"  }catch(e){}"
"}"
"function renderLogList(files){"
"  const el=document.getElementById('log-list'),cnt=document.getElementById('log-list-count');"
"  el.innerHTML='';"
"  if(cnt)cnt.textContent=files.length+(files.length===1?' archivo':' archivos');"
"  if(!files.length){el.innerHTML='<div style=\"color:var(--mut);font-size:12px;padding:8px\">Sin logs disponibles</div>';return;}"
"  files.forEach(f=>{"
"    const d=document.createElement('div');d.className='pi';"
"    const date=f.replace('production_','').replace('.csv','');"
"    d.innerHTML='<div class=\"pic\">'+date+'</div><div class=\"pin\">Sesiones de produccion</div>';"
"    d.onclick=()=>openLog(f,d);el.appendChild(d);"
"  });"
"}"
"async function openLog(file,el){"
"  document.querySelectorAll('#log-list .pi').forEach(e=>e.classList.remove('sel'));"
"  if(el)el.classList.add('sel');"
"  const vw=document.getElementById('log-viewer');"
"  vw.innerHTML='<div style=\"padding:20px;color:var(--mut)\">Cargando...</div>';"
"  try{"
"    const r=await fetch('/api/logs?file='+encodeURIComponent(file));"
"    if(!r.ok){vw.innerHTML='<div style=\"padding:20px;color:var(--red)\">Error al cargar</div>';return;}"
"    const txt=await r.text();renderLogContent(txt,file);"
"  }catch(e){vw.innerHTML='<div style=\"padding:20px;color:var(--red)\">Error de conexion</div>';}"
"}"
"function renderLogContent(csv,filename){"
"  const vw=document.getElementById('log-viewer');"
"  const lines=csv.trim().split('\\n').filter(l=>l.trim());"
"  const title=filename.replace('production_','').replace('.csv','');"
"  if(!lines.length){vw.innerHTML='<div style=\"padding:20px;color:var(--mut)\">Archivo vacio</div>';return;}"
"  const cols=['Inicio','Fin','Perfil','Metros','Vel. prom.','Cortes','Motivo'];"
"  let h='<div style=\"font-size:13px;font-weight:600;margin-bottom:14px\">'+title+' &mdash; '+lines.length+' sesi'+(lines.length===1?'on':'ones')+'</div>';"
"  h+='<div style=\"overflow-x:auto\"><table><thead><tr>';"
"  cols.forEach(c=>h+='<th>'+c+'</th>');h+='</tr></thead><tbody>';"
"  lines.forEach(line=>{const ps=line.split(';').map(s=>s.trim());h+='<tr>';ps.forEach(p=>h+='<td>'+p+'</td>');h+='</tr>';});"
"  h+='</tbody></table></div>';vw.innerHTML=h;"
"}"
"function ltab(el){"  /* kept for compat */
"  document.querySelectorAll('.ltab').forEach(t=>t.classList.remove('act'));"
"  el.classList.add('act');"
"}"

/* --- INIT --- */
"function toggleTheme(){"
"  const l=document.body.getAttribute('data-theme')==='light';"
"  const n=l?'dark':'light';"
"  document.body.setAttribute('data-theme',n);"
"  try{localStorage.setItem('theme',n);}catch(e){}"
"  document.getElementById('theme-btn').textContent=n==='light'?'☀':'🌙';"
"  drawChart();drawSprayArc();"
"}"
"poll();"
"setInterval(poll,2000);"
"window.addEventListener('resize',drawChart);"
"(function(){"
"  try{const t=localStorage.getItem('theme');if(t){document.body.setAttribute('data-theme',t);const b=document.getElementById('theme-btn');if(b)b.textContent=t==='light'?'☀':'🌙';}}catch(e){}"
"})();"
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

    int  spray_rem      = alarm_config_spray_shots_get_remaining();
    int  spray_max      = alarm_config_spray_shots_get_max();
    bool sensor_on      = extrusion_get_sensor_state();
    bool relay_fired    = extrusion_get_relay_fired();
    bool alarm_relay_on = alarm_get_relay_state();

    char *buf = malloc(768);
    if (!buf) return ESP_FAIL;

    snprintf(buf, 768,
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
        "\"ip\":\"%s\","
        "\"spray_remaining\":%d,"
        "\"spray_max\":%d,"
        "\"sensor_correa\":%s,"
        "\"relay_fired\":%s,"
        "\"alarm_relay\":%s"
        "}",
        speed, avg_speed, total_m, cuts, cut_dist, tgt_count,
        recording    ? "true" : "false",
        alarm_active ? "true" : "false",
        active   ? active   : "",
        belt_speed,
        start_t  ? start_t  : "",
        wifi_ok  ? "true" : "false",
        ssid     ? ssid     : "",
        ip       ? ip       : "",
        spray_rem, spray_max,
        sensor_on       ? "true" : "false",
        relay_fired     ? "true" : "false",
        alarm_relay_on  ? "true" : "false"
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
    char results[60][32];
    int count = profile_search(NULL, results, 60);

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

    char imgpath[160];
    snprintf(imgpath, sizeof(imgpath), "/sdcard/profiles/%s.png", code);
    remove(imgpath);

    httpd_resp_sendstr(req, "Deleted");
    return ESP_OK;
}

// =============================================================
// /api/logs/list  — lista de archivos CSV disponibles
// =============================================================
static esp_err_t logs_list_handler(httpd_req_t *req)
{
    DIR *dir = opendir("/sdcard/logs");
    if (!dir)
        return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Logs dir not found");

    char names[64][32];
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && count < 64) {
        if (strncmp(entry->d_name, "production_", 11) != 0) continue;
        strncpy(names[count], entry->d_name, 31);
        names[count][31] = 0;
        count++;
    }
    closedir(dir);

    // Bubble sort descending (newest first)
    for (int i = 0; i < count - 1; i++)
        for (int j = 0; j < count - i - 1; j++)
            if (strcmp(names[j], names[j+1]) < 0) {
                char tmp[32];
                strcpy(tmp, names[j]);
                strcpy(names[j], names[j+1]);
                strcpy(names[j+1], tmp);
            }

    int buf_size = count * 42 + 8;
    char *json = malloc(buf_size);
    if (!json) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No mem");

    strcpy(json, "[");
    for (int i = 0; i < count; i++) {
        if (i > 0) strcat(json, ",");
        strcat(json, "\"");
        strcat(json, names[i]);
        strcat(json, "\"");
    }
    strcat(json, "]");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    free(json);
    return ESP_OK;
}

// =============================================================
// /api/logs  — CSV del día o de un archivo específico (?file=)
// =============================================================
static esp_err_t logs_handler(httpd_req_t *req)
{
    char filepath[128];
    char query[128];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char filename[64];
        if (httpd_query_key_value(query, "file", filename, sizeof(filename)) == ESP_OK) {
            if (strncmp(filename, "production_", 11) != 0 ||
                strstr(filename, "/") || strstr(filename, ".."))
                return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Nombre invalido");
            snprintf(filepath, sizeof(filepath), "/sdcard/logs/%s", filename);
        } else {
            char date[32];
            rtc_get_date_filename_string(date);
            snprintf(filepath, sizeof(filepath), "/sdcard/logs/production_%s.csv", date);
        }
    } else {
        char date[32];
        rtc_get_date_filename_string(date);
        snprintf(filepath, sizeof(filepath), "/sdcard/logs/production_%s.csv", date);
    }

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
// /api/profile/image  GET / POST / DELETE
// =============================================================
static esp_err_t profile_image_get_handler(httpd_req_t *req)
{
    char query[128], code[64], filepath[160];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta query");
    if (httpd_query_key_value(query, "code", code, sizeof(code)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta code");

    snprintf(filepath, sizeof(filepath), "/sdcard/profiles/%s.png", code);

    FILE *f = fopen(filepath, "rb");
    if (!f)
        return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Imagen no encontrada");

    httpd_resp_set_type(req, "image/png");
    httpd_resp_set_hdr(req, "Cache-Control", "max-age=30");

    char chunk[1024];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0)
        httpd_resp_send_chunk(req, chunk, n);

    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t profile_image_upload_handler(httpd_req_t *req)
{
    char query[128], code[64], filepath[160];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta query");
    if (httpd_query_key_value(query, "code", code, sizeof(code)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta code");

    int total = req->content_len;
    if (total <= 0 || total > 512 * 1024)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Tamaño invalido (max 512 KB)");

    snprintf(filepath, sizeof(filepath), "/sdcard/profiles/%s.png", code);

    FILE *f = fopen(filepath, "wb");
    if (!f)
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error al crear imagen");

    char buf[512];
    int remaining = total;
    bool ok = true;

    while (remaining > 0) {
        int n = httpd_req_recv(req, buf,
                               (remaining < (int)sizeof(buf)) ? remaining : (int)sizeof(buf));
        if (n <= 0) { ok = false; break; }
        fwrite(buf, 1, n, f);
        remaining -= n;
    }
    fclose(f);

    if (!ok) {
        remove(filepath);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Imagen guardada: %s (%d bytes)", filepath, total);
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

static esp_err_t profile_image_delete_handler(httpd_req_t *req)
{
    char query[128], code[64], filepath[160];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta query");
    if (httpd_query_key_value(query, "code", code, sizeof(code)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta code");

    snprintf(filepath, sizeof(filepath), "/sdcard/profiles/%s.png", code);

    if (remove(filepath) != 0)
        return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Imagen no encontrada");

    httpd_resp_sendstr(req, "Deleted");
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
static esp_err_t ota_status_handler(httpd_req_t *req)
{
    ota_status_t st;
    ota_get_status(&st);

    char json[1024];
    snprintf(json, sizeof(json),
        "{"
        "\"current_version\":\"%s\","
        "\"available_version\":\"%s\","
        "\"update_available\":%s,"
        "\"state\":\"%s\","
        "\"progress\":%d,"
        "\"busy\":%s,"
        "\"last_error\":\"%s\""
        "}",
        st.current_version,
        st.available_version,
        st.update_available ? "true" : "false",
        ota_state_to_str(st.state),
        st.progress,
        st.busy ? "true" : "false",
        st.last_error);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t ota_check_handler(httpd_req_t *req)
{
    esp_err_t err = ota_check_for_update();
    if (err == ESP_ERR_INVALID_STATE) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA busy");
    }
    if (err != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "check failed");
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true,\"message\":\"check started\"}");
    return ESP_OK;
}

static esp_err_t ota_start_handler(httpd_req_t *req)
{
    esp_err_t err = ota_start_update();
    if (err == ESP_ERR_INVALID_STATE) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA busy");
    }
    if (err != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "start failed");
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true,\"message\":\"update started\"}");
    return ESP_OK;
}

// =============================================================
// /api/test?relay=alarm|paint  — prueba manual de relays
// =============================================================
static esp_err_t relay_test_handler(httpd_req_t *req)
{
    char query[64], target[16];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta query");

    if (httpd_query_key_value(query, "relay", target, sizeof(target)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Falta relay");

    if (strcmp(target, "alarm") == 0) {
        alarm_trigger_speed();
    } else if (strcmp(target, "paint") == 0) {
        extrusion_relay_test();
    } else {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "relay invalido");
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

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
    config.max_uri_handlers = 24;
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
        { .uri = "/api/logs",      .method = HTTP_GET, .handler = logs_handler      },
        { .uri = "/api/logs/list", .method = HTTP_GET, .handler = logs_list_handler },
        { .uri = "/api/logs/all",  .method = HTTP_GET, .handler = logs_all_handler  },
        { .uri = "/api/profile/image", .method = HTTP_GET,    .handler = profile_image_get_handler    },
        { .uri = "/api/profile/image", .method = HTTP_POST,   .handler = profile_image_upload_handler },
        { .uri = "/api/profile/image", .method = HTTP_DELETE, .handler = profile_image_delete_handler },
        { .uri = "/api/ota/status", .method = HTTP_GET,  .handler = ota_status_handler },
        { .uri = "/api/ota/check",  .method = HTTP_POST, .handler = ota_check_handler  },
        { .uri = "/api/ota/start",  .method = HTTP_POST, .handler = ota_start_handler  },
        { .uri = "/api/test",       .method = HTTP_GET,  .handler = relay_test_handler  },
    };

    for (int i = 0; i < (int)(sizeof(routes) / sizeof(routes[0])); i++)
        httpd_register_uri_handler(server, &routes[i]);

    ESP_LOGI(TAG, "HTTP server started");
}
