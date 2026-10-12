param([string]$Executable = (Join-Path $PSScriptRoot '..\Praat-custom.exe'))
$ErrorActionPreference = 'Stop'

Add-Type @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class TrajectoryEditorWindows {
    public delegate bool EnumProc(IntPtr window, IntPtr parameter);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc callback, IntPtr parameter);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr window, StringBuilder text, int capacity);
}
'@

function Test-TrajectoryEditor([string]$RelativeScript, [string]$TitlePattern) {
    $scriptPath = (Resolve-Path (Join-Path $PSScriptRoot "..\$RelativeScript")).Path
    $process = Start-Process -FilePath $Executable -ArgumentList @('--new-send', '--no-pref-files', '--no-plugins', '--FULL-TRUST', $scriptPath) -PassThru -WindowStyle Hidden
    try {
        $script:trajectoryFound = $false
        $deadline = [DateTime]::UtcNow.AddSeconds(20)
        while (-not $script:trajectoryFound -and [DateTime]::UtcNow -lt $deadline) {
            [TrajectoryEditorWindows]::EnumWindows({
                param($window, $unused)
                [uint32]$owner = 0
                [void][TrajectoryEditorWindows]::GetWindowThreadProcessId($window, [ref]$owner)
                if ($owner -ne $process.Id) { return $true }
                $title = [System.Text.StringBuilder]::new(256)
                [void][TrajectoryEditorWindows]::GetWindowText($window, $title, $title.Capacity)
                if ($title.ToString() -match $TitlePattern) {
                    $script:trajectoryFound = $true
                    return $false
                }
                return $true
            }, [IntPtr]::Zero) | Out-Null
            if (-not $script:trajectoryFound) { Start-Sleep -Milliseconds 150 }
        }
        if (-not $script:trajectoryFound) { throw "Trajectory editor window did not open for $RelativeScript" }
        if ($process.HasExited) { throw "Praat exited unexpectedly for $RelativeScript" }
    } finally {
        if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
        $process.Dispose()
    }
}

Test-TrajectoryEditor 'tests\trajectories\gui-smart.praat' 'FrequencyTrajectories.*smart_overlay'
Test-TrajectoryEditor 'tests\trajectories\demo.praat' 'FrequencyTrajectories.*tracks'
Write-Output 'TRAJECTORIES GUI: PASS'
