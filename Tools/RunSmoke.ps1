# 게임을 창 모드로 띄워 오토플레이 봇이 플레이하게 하고, 로그 요약과 스크린샷을 남긴다.
# 스크린샷: Saved/Screenshots/AutoPlay_0..2.png
param(
    [string]$EngineDir = "D:\source\repos\UnrealEngine\Engine",
    [int]$Seconds = 30,
    [int]$ResX = 540,
    [int]$ResY = 960
)

$ProjectDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$Project = Join-Path $ProjectDir "YJJJump.uproject"
$Editor = Join-Path $EngineDir "Binaries\Win64\UnrealEditor.exe"
$Log = Join-Path $ProjectDir "Saved\Logs\RunSmoke.log"

Remove-Item (Join-Path $ProjectDir "Saved\Screenshots\AutoPlay_*.png") -ErrorAction SilentlyContinue

$Args = @(
    "`"$Project`"", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY", "-WinX=200", "-WinY=60", "-nosplash", "-NoSound", "-unattended",
    "-ExecCmds=`"yjj.AutoPlay 1, yjj.AutoPlayDuration $Seconds`"", "-log", "-abslog=`"$Log`""
)
$Process = Start-Process -FilePath $Editor -ArgumentList $Args -PassThru
if (-not $Process.WaitForExit(($Seconds + 900) * 1000)) {
    $Process.Kill()
    Write-Host "Timed out"
}

Select-String -Path $Log -Pattern "\[AutoPlay\]|Error:|Ensure condition failed|Fatal" | ForEach-Object { $_.Line }
Get-ChildItem (Join-Path $ProjectDir "Saved\Screenshots\AutoPlay_*.png") -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName }
