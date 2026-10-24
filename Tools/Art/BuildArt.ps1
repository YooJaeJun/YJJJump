# 픽셀아트 PNG와 BGM/효과음 WAV를 생성하고 언리얼 에셋으로 임포트한다.
# 에디터를 끈 상태에서 실행: powershell -File Tools/Art/BuildArt.ps1
param(
    [string]$EngineDir = "D:\source\repos\UnrealEngine\Engine"
)

$ErrorActionPreference = "Stop"
$ProjectDir = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$Project = Join-Path $ProjectDir "YJJJump.uproject"
$Python = Join-Path $EngineDir "Binaries\ThirdParty\Python3\Win64\python.exe"
$Editor = Join-Path $EngineDir "Binaries\Win64\UnrealEditor-Cmd.exe"
$Generated = Join-Path $PSScriptRoot "Generated"

& $Python -I (Join-Path $PSScriptRoot "generate_art.py") $Generated
if ($LASTEXITCODE -ne 0) { throw "generate_art.py failed" }
& $Python -I (Join-Path $PSScriptRoot "..\Audio\generate_audio.py") (Join-Path $PSScriptRoot "..\Audio\Generated")
if ($LASTEXITCODE -ne 0) { throw "generate_audio.py failed" }

$Script = (Join-Path $PSScriptRoot "import_assets.py") -replace '\\', '/'
& $Editor $Project -run=pythonscript -script="$Script" -unattended -nosplash -nop4 -nullrhi -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0) { throw "import_assets.py failed ($LASTEXITCODE)" }
