<#
.SYNOPSIS
    Toma capturas automaticas del simulador para el Manual de Usuario.
.DESCRIPTION
    Ejecutar desde PowerShell (no desde MSYS2):
        powershell -ExecutionPolicy Bypass -File tomar_capturas.ps1
#>

$ROOT    = "D:\00_PROYECTOS\rubber-srl\codigo\extrusion-controller"
$SIM_EXE = "$ROOT\simulator\build\extrusion_simulator.exe"
$OUT_DIR = "$ROOT\docs\images\capturas"
$env:PATH = "C:\msys64\mingw64\bin;" + $env:PATH

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32 {
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, ref RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT p);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);
    public const uint WM_MOUSEMOVE   = 0x0200;
    public const uint WM_LBUTTONDOWN = 0x0201;
    public const uint WM_LBUTTONUP   = 0x0202;
    public const uint MK_LBUTTON     = 0x0001;
    public const uint PW_CLIENTONLY  = 0x01;
    public const uint PW_RENDERFULL  = 0x02;
    [StructLayout(LayoutKind.Sequential)] public struct RECT  { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
}
"@

$script:hwnd = [IntPtr]::Zero

function MakeLParam([int]$x, [int]$y) {
    return [IntPtr](($y -shl 16) -bor ($x -band 0xFFFF))
}

function Screenshot {
    param([string]$name, [int]$cx=0, [int]$cy=0, [int]$cw=800, [int]$ch=480)
    $rect = New-Object Win32+RECT
    [Win32]::GetClientRect($script:hwnd, [ref]$rect) | Out-Null
    $clientW = [Math]::Max($rect.Right - $rect.Left, 1)
    $clientH = [Math]::Max($rect.Bottom - $rect.Top, 1)
    $fullBmp = New-Object System.Drawing.Bitmap($clientW, $clientH)
    $g = [System.Drawing.Graphics]::FromImage($fullBmp)
    [Win32]::PrintWindow($script:hwnd, $g.GetHdc(), ([Win32]::PW_CLIENTONLY -bor [Win32]::PW_RENDERFULL)) | Out-Null
    $g.ReleaseHdc(); $g.Dispose()
    $isFull = ($cx -eq 0 -and $cy -eq 0 -and $cw -ge $clientW -and $ch -ge $clientH)
    if ($isFull) {
        $outBmp = $fullBmp
    } else {
        $aw = [Math]::Min($cw, $clientW - $cx); $ah = [Math]::Min($ch, $clientH - $cy)
        $outBmp = New-Object System.Drawing.Bitmap($aw, $ah)
        $g2 = [System.Drawing.Graphics]::FromImage($outBmp)
        $g2.DrawImage($fullBmp, (New-Object System.Drawing.Rectangle(0,0,$aw,$ah)),
                      (New-Object System.Drawing.Rectangle($cx,$cy,$aw,$ah)),
                      [System.Drawing.GraphicsUnit]::Pixel)
        $g2.Dispose(); $fullBmp.Dispose()
    }
    $path = Join-Path $OUT_DIR ($name + ".png")
    $outBmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $outBmp.Dispose()
    Write-Host ("  [OK] " + $name + ".png")
}

function Click {
    param([int]$cx, [int]$cy, [int]$delayMs=350)
    $lp  = MakeLParam $cx $cy
    $wp0 = [IntPtr]::Zero
    $wp1 = [IntPtr][uint32][Win32]::MK_LBUTTON
    [Win32]::PostMessage($script:hwnd, [Win32]::WM_MOUSEMOVE,   $wp0, $lp) | Out-Null
    Start-Sleep -Milliseconds 60
    [Win32]::PostMessage($script:hwnd, [Win32]::WM_LBUTTONDOWN, $wp1, $lp) | Out-Null
    Start-Sleep -Milliseconds 80
    [Win32]::PostMessage($script:hwnd, [Win32]::WM_LBUTTONUP,   $wp0, $lp) | Out-Null
    Start-Sleep -Milliseconds $delayMs
}

# Drag hacia arriba terminando fuera del cliente (endY negativo) para que LVGL
# no dispare click en ningun widget al soltar
function DragUp {
    param([int]$cx, [int]$startY, [int]$pixels, [int]$delayMs=500)
    $endY = $startY - $pixels
    $wp0  = [IntPtr]::Zero
    $wp1  = [IntPtr][uint32][Win32]::MK_LBUTTON
    $lpS  = MakeLParam $cx $startY
    [Win32]::PostMessage($script:hwnd, [Win32]::WM_MOUSEMOVE,   $wp0, $lpS) | Out-Null
    Start-Sleep -Milliseconds 50
    [Win32]::PostMessage($script:hwnd, [Win32]::WM_LBUTTONDOWN, $wp1, $lpS) | Out-Null
    Start-Sleep -Milliseconds 50
    $steps = 10
    for ($i = 1; $i -le $steps; $i++) {
        $midY = [int]($startY - $pixels * $i / $steps)
        $lpM  = MakeLParam $cx $midY
        [Win32]::PostMessage($script:hwnd, [Win32]::WM_MOUSEMOVE, $wp1, $lpM) | Out-Null
        Start-Sleep -Milliseconds 20
    }
    $lpE = MakeLParam $cx $endY
    [Win32]::PostMessage($script:hwnd, [Win32]::WM_LBUTTONUP, $wp0, $lpE) | Out-Null
    Start-Sleep -Milliseconds $delayMs
}

# Coordenadas LVGL (800x480)
$TAB_X=66; $TAB_EXTRUDIR=102; $TAB_PERFILES=210; $TAB_AJUSTES=318
$CTAB_Y=72; $CTAB_FECHA=215; $CTAB_MAQUIN=549; $CTAB_SIST=716
$GRABAR_X=266; $GRABAR_Y=404
$REFRESH_X=762; $REFRESH_Y=100
$ITEM1_X=466; $ITEM1_Y=175
$SELEC_X=212; $SELEC_Y=441

# Lanzar simulador
Write-Host ""
Write-Host "Lanzando simulador..."
if (-not (Test-Path $SIM_EXE)) { Write-Error ("No se encontro: " + $SIM_EXE); exit 1 }
$proc = Start-Process -FilePath $SIM_EXE -PassThru -ErrorAction Stop
for ($i = 0; $i -lt 20; $i++) {
    Start-Sleep -Seconds 1; $proc.Refresh()
    if ($proc.MainWindowHandle -ne [IntPtr]::Zero) { break }
}
if ($proc.MainWindowHandle -eq [IntPtr]::Zero) {
    Write-Error "El simulador no levanto ventana."; $proc | Stop-Process -Force; exit 1
}
$script:hwnd = $proc.MainWindowHandle
[Win32]::SetForegroundWindow($script:hwnd) | Out-Null
Start-Sleep -Seconds 2

Write-Host "Simulador activo. Tomando capturas..."

# 1. Extruir inicial
Write-Host ""; Write-Host "--- Pantalla Extruir ---"
Screenshot "pantalla-extruir"
Screenshot "barra-de-estado-superior" -cx 0 -cy 0  -cw 800 -ch 48
Screenshot "menu-de-navegacion"       -cx 0 -cy 48 -cw 132 -ch 432

# 2. Perfiles
Write-Host "--- Lista de Perfiles ---"
Click $TAB_X $TAB_PERFILES 800
Screenshot "lista-de-perfiles"

# 3. Grabando
Write-Host "--- Extruir Grabando ---"
Click $ITEM1_X $ITEM1_Y 800
Screenshot "perfil-detalle-modal"
Click $SELEC_X $SELEC_Y 1000
Screenshot "pantalla-extruir-con-perfil"
Click $GRABAR_X $GRABAR_Y 600
Screenshot "pantalla-extruir-grabando"

# 4. Configuracion
# Delay 1200ms: production_finish llama lv_delay_ms(500) por el relay de marcado
Write-Host "--- Configuracion ---"
Click $GRABAR_X $GRABAR_Y 1200
Click $TAB_X $TAB_AJUSTES 800
Screenshot "pantalla-configuracion"

# 5. Maquina: 3 drags de 220px hacia arriba terminando fuera del cliente
Write-Host "--- Ajustes Maquina ---"
Click $CTAB_MAQUIN $CTAB_Y 600
Screenshot "subpestana-maquina"
DragUp 466 450 580 600
Screenshot "contador-spray"

# 6. Sistema
Write-Host "--- Ajustes Sistema ---"
Click $CTAB_SIST $CTAB_Y 600
Screenshot "subpestana-sistema"

Write-Host ""
Write-Host "[NOTA] 'indicador-alarma': activa alarma en Ajustes->Maquina, luego graba."

Write-Host ""; Write-Host "Cerrando simulador..."
$proc | Stop-Process -Force; Start-Sleep -Milliseconds 500

Write-Host ""; Write-Host ("Capturas en: " + $OUT_DIR)
Get-ChildItem $OUT_DIR -Filter "*.png" | ForEach-Object { Write-Host ("  " + $_.Name) }