# YJJJump 자동화 테스트를 헤드리스로 실행하고 결과를 요약한다.
# 에디터를 끈 상태에서 실행: powershell -File Tools/RunTests.ps1
param(
    [string]$EngineDir = "D:\source\repos\UnrealEngine\Engine",
    [string]$Filter = "YJJJump"
)

$ProjectDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$Project = Join-Path $ProjectDir "YJJJump.uproject"
$Editor = Join-Path $EngineDir "Binaries\Win64\UnrealEditor-Cmd.exe"
$Log = Join-Path $ProjectDir "Saved\Logs\RunTests.log"

& $Editor $Project -ExecCmds="Automation RunTests $Filter;Quit" -unattended -nosplash -nop4 -nullrhi -NoSound -stdout -FullStdOutLogOutput *> $Log

$Results = Select-String -Path $Log -Pattern "Test Completed\. Result=\{(\w+)\} Name=\{([^}]*)\}" | ForEach-Object {
    [pscustomobject]@{ Result = $_.Matches[0].Groups[1].Value; Name = $_.Matches[0].Groups[2].Value }
}
$Results | Format-Table -AutoSize

$Errors = Select-String -Path $Log -Pattern "LogAutomationController: Error" | ForEach-Object { $_.Line }
$Errors | Select-Object -First 30

$Failed = @($Results | Where-Object { $_.Result -ne "Success" }).Count
if ($Results.Count -eq 0) { Write-Host "No tests ran. See $Log"; exit 2 }
Write-Host "$($Results.Count) test(s), $Failed failed. Log: $Log"
exit [int]($Failed -gt 0)
