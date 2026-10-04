param([string]$Executable = (Join-Path $PSScriptRoot '..\Praat-custom.exe'))
$ErrorActionPreference = 'Stop'

Add-Type @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class ScriptStopWindow {
    public delegate bool EnumProc(IntPtr window, IntPtr parameter);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc callback, IntPtr parameter);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetClassName(IntPtr window, StringBuilder name, int capacity);
    [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);
}
'@

$scratch = Join-Path $PSScriptRoot '..\.local-build\script-stop'
New-Item -ItemType Directory -Path $scratch -Force | Out-Null
$cases = @(
    @{ Name = 'Lua'; File = 'loop.lua'; Source = "while true do end`n" },
    @{ Name = 'Praat'; File = 'loop.praat'; Source = "while 1`nendwhile`n" }
)
foreach ($case in $cases) {
    $script = Join-Path $scratch $case.File
    [System.IO.File]::WriteAllText($script, $case.Source, [System.Text.UTF8Encoding]::new($false))
    $process = Start-Process -FilePath $Executable -ArgumentList @('--new-send', '--no-pref-files', '--no-plugins', $script) -PassThru -WindowStyle Hidden
    try {
        $window = [IntPtr]::Zero
        $deadline = [DateTime]::UtcNow.AddSeconds(15)
        while ($window -eq [IntPtr]::Zero -and [DateTime]::UtcNow -lt $deadline) {
            [ScriptStopWindow]::EnumWindows({
                param($candidate, $unused)
                [uint32]$owner = 0
                [void][ScriptStopWindow]::GetWindowThreadProcessId($candidate, [ref]$owner)
                $name = [System.Text.StringBuilder]::new(128)
                [void][ScriptStopWindow]::GetClassName($candidate, $name, $name.Capacity)
                if ($owner -eq $process.Id -and $name.ToString() -eq 'PraatScriptStopWindow') {
                    $script:window = $candidate
                    return $false
                }
                return $true
            }, [IntPtr]::Zero) | Out-Null
            if ($window -eq [IntPtr]::Zero) { Start-Sleep -Milliseconds 100 }
        }
        if ($window -eq [IntPtr]::Zero) { throw "$($case.Name): Stop window did not appear" }
        [void][ScriptStopWindow]::SendMessage($window, 0x0111, [IntPtr]1, [IntPtr]::Zero)
        $deadline = [DateTime]::UtcNow.AddSeconds(10)
        while ([DateTime]::UtcNow -lt $deadline) {
            $found = $false
            [ScriptStopWindow]::EnumWindows({
                param($candidate, $unused)
                if ($candidate -eq $window) { $script:found = $true; return $false }
                return $true
            }, [IntPtr]::Zero) | Out-Null
            if (-not $found) { break }
            Start-Sleep -Milliseconds 100
        }
        if ($found) { throw "$($case.Name): Stop window did not close" }
        if ($process.HasExited) { throw "$($case.Name): Praat process exited" }
        Write-Output "$($case.Name) STOP: PASS"
    } finally {
        if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
        $process.Dispose()
    }
}
