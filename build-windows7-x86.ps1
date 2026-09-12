param(
    [string]$MsysRoot = 'C:\msys64',
    [ValidateRange(1,64)][int]$Jobs = 2
)
$ErrorActionPreference = 'Stop'
$bash = Join-Path $MsysRoot 'usr\bin\bash.exe'
if (-not (Test-Path -LiteralPath $bash)) { throw "MSYS2 not found: $MsysRoot" }
$previousSystem = $env:MSYSTEM
$previousChere = $env:CHERE_INVOKING
$previousJobs = $env:JOBS
$previousGit = $env:PRAAT_GIT
Push-Location $PSScriptRoot
try {
    $env:MSYSTEM = 'MINGW32'
    $env:CHERE_INVOKING = '1'
    $env:JOBS = [string]$Jobs
    $env:PRAAT_GIT = (Get-Command git -CommandType Application | Select-Object -First 1).Source
    & $bash --login -c 'bash scripts/build-windows7-x86.sh'
    if ($LASTEXITCODE -ne 0) { throw "Win7 x86 build failed ($LASTEXITCODE). See .local-build/win7-x86-build.log." }
    Compress-Archive -Path (Join-Path $PSScriptRoot 'dist\windows7-x86\*') -DestinationPath (Join-Path $PSScriptRoot 'dist\praat-win7-x86.zip') -Force
    Write-Output 'Ready: dist\praat-win7-x86.zip'
} finally {
    Pop-Location
    $env:MSYSTEM = $previousSystem
    $env:CHERE_INVOKING = $previousChere
    $env:JOBS = $previousJobs
    $env:PRAAT_GIT = $previousGit
}
