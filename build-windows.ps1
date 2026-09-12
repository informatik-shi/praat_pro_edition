param(
    [string]$MsysRoot = 'C:\msys64',
    [ValidateRange(1,64)][int]$Jobs = 4,
    [switch]$Test
)
$ErrorActionPreference = 'Stop'
$bash = Join-Path $MsysRoot 'usr\bin\bash.exe'
if (-not (Test-Path -LiteralPath $bash)) { throw "MSYS2 not found at $MsysRoot. See BUILD_WINDOWS.md." }
$previousSystem = $env:MSYSTEM
$previousChere = $env:CHERE_INVOKING
$previousJobs = $env:JOBS
Push-Location $PSScriptRoot
try {
    $env:MSYSTEM = 'CLANG64'
    $env:CHERE_INVOKING = '1'
    $env:JOBS = [string]$Jobs
    & $bash --login -c 'bash scripts/build-windows.sh'
    if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE). See .local-build/build-windows.log." }
    if ($Test) {
        & $bash --login -c 'bash scripts/test-folding.sh && bash scripts/test-lua-debugger.sh && bash scripts/test-lua.sh && bash scripts/test-trajectories.sh && bash scripts/test-debugger.sh && bash scripts/test-windows.sh'
        if ($LASTEXITCODE -ne 0) { throw "Tests failed ($LASTEXITCODE). See .local-build/." }
    }
    Write-Output 'Built: dist\praat-custom.exe'
} finally {
    Pop-Location
    $env:MSYSTEM = $previousSystem
    $env:CHERE_INVOKING = $previousChere
    $env:JOBS = $previousJobs
}
