param([string]$Executable = (Join-Path $PSScriptRoot '..\Praat-custom.exe'))
$ErrorActionPreference = 'Stop'

Add-Type @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class SmartEditorWindows {
    public delegate bool EnumProc(IntPtr window, IntPtr parameter);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc callback, IntPtr parameter);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr window, StringBuilder text, int capacity);
}
'@

$scriptPath = (Resolve-Path (Join-Path $PSScriptRoot '..\tests\smart-spectrogram\gui.praat')).Path
$process = Start-Process -FilePath $Executable -ArgumentList @('--new-send', '--no-pref-files', '--no-plugins', '--FULL-TRUST', $scriptPath) -PassThru -WindowStyle Hidden
try {
    $found = $false
    $deadline = [DateTime]::UtcNow.AddSeconds(20)
    while (-not $found -and [DateTime]::UtcNow -lt $deadline) {
        [SmartEditorWindows]::EnumWindows({
            param($window, $unused)
            [uint32]$owner = 0
            [void][SmartEditorWindows]::GetWindowThreadProcessId($window, [ref]$owner)
            if ($owner -ne $process.Id) { return $true }
            $title = [System.Text.StringBuilder]::new(256)
            [void][SmartEditorWindows]::GetWindowText($window, $title, $title.Capacity)
            if ($title.ToString() -match 'SmartSpectrogram.*irregular') {
                $script:found = $true
                return $false
            }
            return $true
        }, [IntPtr]::Zero) | Out-Null
        if (-not $found) { Start-Sleep -Milliseconds 150 }
    }
    if (-not $found) { throw 'SmartSpectrogram editor window did not open' }
    if ($process.HasExited) { throw 'Praat exited unexpectedly' }
    Write-Output 'SMART SPECTROGRAM GUI: PASS'
} finally {
    if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
    $process.Dispose()
}
