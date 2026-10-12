param([string]$Executable = (Join-Path $PSScriptRoot '..\dist\praat-custom.exe'))
$ErrorActionPreference = 'Stop'

Add-Type @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class ProEditorWindows {
    public delegate bool EnumProc(IntPtr window, IntPtr parameter);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc callback, IntPtr parameter);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr window, StringBuilder text, int capacity);
}
'@

foreach ($name in @('gui-combined-three.praat', 'gui-combined-five.praat', 'gui-combined-settings.praat', 'gui-sound-spectrogram.praat')) {
    $withSettings = $name -in @('gui-combined-settings.praat', 'gui-sound-spectrogram.praat')
    $markerName = if ($name -eq 'gui-sound-spectrogram.praat') { 'gui-sound-spectrogram.ok' } else { 'gui-combined-settings.ok' }
    $marker = Join-Path $PSScriptRoot "..\.local-build\$markerName"
    if ($withSettings -and (Test-Path -LiteralPath $marker)) {
        Remove-Item -LiteralPath $marker
    }
    $scriptPath = (Resolve-Path (Join-Path $PSScriptRoot "..\tests\trajectories\$name")).Path
    $process = Start-Process -FilePath $Executable -ArgumentList @('--new-send', '--no-pref-files', '--no-plugins', '--FULL-TRUST', $scriptPath) -PassThru -WindowStyle Hidden
    try {
        $script:found = $false
        $deadline = [DateTime]::UtcNow.AddSeconds(25)
        while (-not $found -and [DateTime]::UtcNow -lt $deadline) {
            [ProEditorWindows]::EnumWindows({
                param($window, $unused)
                [uint32]$owner = 0
                [void][ProEditorWindows]::GetWindowThreadProcessId($window, [ref]$owner)
                if ($owner -ne $process.Id) { return $true }
                $title = [System.Text.StringBuilder]::new(256)
                [void][ProEditorWindows]::GetWindowText($window, $title, $title.Capacity)
                $expectedTitle = if ($name -eq 'gui-sound-spectrogram.praat') { 'Sound sound_spectrogram' } else { 'Sound .*combined_.*selected layers' }
                if ($title.ToString() -match $expectedTitle) {
                    $script:found = $true
                    return $false
                }
                return $true
            }, [IntPtr]::Zero) | Out-Null
            if (-not $found) { Start-Sleep -Milliseconds 150 }
        }
        if (-not $found) { throw "Combined editor did not open for $name" }
        if ($process.HasExited) { throw "Praat exited unexpectedly for $name" }
        if ($withSettings) {
            $settingsDeadline = [DateTime]::UtcNow.AddSeconds(10)
            while (-not (Test-Path -LiteralPath $marker) -and [DateTime]::UtcNow -lt $settingsDeadline) {
                Start-Sleep -Milliseconds 150
            }
            if (-not (Test-Path -LiteralPath $marker)) {
                throw 'Combined editor did not apply spectrogram settings and switch layers'
            }
        }
    } finally {
        if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
        $process.Dispose()
    }
}
$renderScript = (Resolve-Path (Join-Path $PSScriptRoot '..\tests\trajectories\render-sound-spectrogram.praat')).Path
$renderedPng = Join-Path $PSScriptRoot '..\.local-build\spectrogram-pixels.png'
$narrowPng = Join-Path $PSScriptRoot '..\.local-build\spectrogram-narrow.png'
if (Test-Path -LiteralPath $renderedPng) { Remove-Item -LiteralPath $renderedPng }
if (Test-Path -LiteralPath $narrowPng) { Remove-Item -LiteralPath $narrowPng }
$render = Start-Process -FilePath $Executable -ArgumentList @('--utf8', '--no-pref-files', '--no-plugins', '--FULL-TRUST', '--run', $renderScript) -PassThru -Wait -WindowStyle Hidden
try {
    if ($render.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $renderedPng) -or -not (Test-Path -LiteralPath $narrowPng)) {
        throw 'Sound spectrogram PNG was not rendered'
    }
    Add-Type -AssemblyName System.Drawing
    $bitmap = [System.Drawing.Bitmap]::FromFile($renderedPng)
    $narrow = [System.Drawing.Bitmap]::FromFile($narrowPng)
    try {
        $bright = $bitmap.GetPixel(500, 700).R
        $dark = $bitmap.GetPixel(500, 1000).R
        $narrowBright = $narrow.GetPixel(500, 700).R
        if ($bright -le $dark + 50) { throw "Sound spectrogram has no visible frequency contrast ($bright / $dark)" }
        if ($narrowBright -le $bright + 30) { throw "Changing the dynamic range did not change the spectrogram ($narrowBright / $bright)" }
    } finally {
        $bitmap.Dispose()
        $narrow.Dispose()
    }
} finally {
    $render.Dispose()
}
Write-Output 'PRO EDITOR GUI AND SPECTROGRAM RENDER: PASS'
