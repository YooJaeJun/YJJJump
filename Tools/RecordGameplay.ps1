# 게임을 창 모드로 띄워 오토플레이 봇이 플레이하는 화면을 ffmpeg로 녹화한다 (영상 소재용).
# 결과: DevLog/Footage/Raw/<이름>.mp4  (창 테두리 포함, 편집할 때 잘라냄)
param(
    [string]$EngineDir = "D:\source\repos\UnrealEngine\Engine",
    [int]$Seconds = 40,
    [int]$ResX = 540,
    [int]$ResY = 960,
    [string]$Name = "gameplay"
)

$ErrorActionPreference = "Stop"
$ProjectDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$Project = Join-Path $ProjectDir "YJJJump.uproject"
$Editor = Join-Path $EngineDir "Binaries\Win64\UnrealEditor.exe"
$OutDir = Join-Path $ProjectDir "DevLog\Footage\Raw"
$Out = Join-Path $OutDir "$Name.mp4"
New-Item -ItemType Directory -Force $OutDir | Out-Null

$FFmpeg = (Get-Command ffmpeg -ErrorAction SilentlyContinue).Source
if (-not $FFmpeg) {
    $FFmpeg = Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Recurse -Filter ffmpeg.exe -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
}
if (-not $FFmpeg) { throw "ffmpeg not found" }

# 로딩 시간 동안은 봇이 안 움직이도록 녹화 시간보다 넉넉히 플레이
$PlaySeconds = $Seconds + 6
$Args = @(
    "`"$Project`"", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY", "-nosplash", "-NoSound", "-unattended",
    "-ExecCmds=`"yjj.AutoPlay 1, yjj.AutoPlayDuration $PlaySeconds`""
)
$Game = Start-Process -FilePath $Editor -ArgumentList $Args -PassThru

# 게임 창이 뜰 때까지 대기
$Title = $null
for ($i = 0; $i -lt 240 -and -not $Title; $i++) {
    Start-Sleep -Milliseconds 500
    $Game.Refresh()
    if ($Game.MainWindowTitle -and $Game.MainWindowTitle -notmatch "Loading") { $Title = $Game.MainWindowTitle }
}
if (-not $Title) { $Game.Kill(); throw "Game window not found" }
Start-Sleep -Seconds 3

# D3D12 창은 GDI 캡처(gdigrab)로 검게 나온다 -> 데스크톱 복제(ddagrab)로 창 영역만 잘라 녹화
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class YJJWin32 {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT point);
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
}
"@
$Hwnd = $Game.MainWindowHandle
[YJJWin32]::SetForegroundWindow($Hwnd) | Out-Null
$Client = New-Object YJJWin32+RECT
[YJJWin32]::GetClientRect($Hwnd, [ref]$Client) | Out-Null
$Origin = New-Object YJJWin32+POINT
[YJJWin32]::ClientToScreen($Hwnd, [ref]$Origin) | Out-Null
# 짝수 크기여야 H.264 인코딩 가능
$W = ($Client.Right - $Client.Left) -band -2
$H = ($Client.Bottom - $Client.Top) -band -2
$Grab = "ddagrab=output_idx=0:framerate=30:draw_mouse=0:offset_x={0}:offset_y={1}:video_size={2}x{3}" -f $Origin.X, $Origin.Y, $W, $H

& $FFmpeg -y -hide_banner -loglevel error -f lavfi -i $Grab -t $Seconds -vf "hwdownload,format=bgra" -c:v libx264 -preset veryfast -crf 18 -pix_fmt yuv420p $Out
$Game.WaitForExit(120000) | Out-Null
if (-not $Game.HasExited) { $Game.Kill() }
Write-Host "Recorded $Out (window '$Title')"
