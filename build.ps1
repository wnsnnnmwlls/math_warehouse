$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

if (-not (Test-Path 'build')) {
    New-Item -ItemType Directory -Path 'build' | Out-Null
}

cmake -S . -B build -G 'MinGW Makefiles' -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build build -j 4
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ''
Write-Host 'Build complete:' (Join-Path $root 'MathPaperStudio.exe')
Write-Host 'Self-test:' (Join-Path $root 'build\selftest.exe')
