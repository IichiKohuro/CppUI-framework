[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Exe
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public static class CppUiDpiNative {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct MONITORINFOEX {
        public int cbSize;
        public RECT rcMonitor;
        public RECT rcWork;
        public uint dwFlags;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string szDevice;
    }

    public sealed class MonitorEntry {
        public string Device;
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
        public uint Dpi;
    }

    public delegate bool MonitorProc(IntPtr hMon, IntPtr hdc, ref RECT rc, IntPtr data);

    [DllImport("user32.dll")]
    public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);

    [DllImport("user32.dll")]
    public static extern bool EnumDisplayMonitors(IntPtr hdc, IntPtr clip, MonitorProc proc, IntPtr data);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern bool GetMonitorInfoW(IntPtr hMon, ref MONITORINFOEX info);

    [DllImport("shcore.dll")]
    public static extern int GetDpiForMonitor(IntPtr hMon, int dpiType, out uint dpiX, out uint dpiY);

    [DllImport("user32.dll")]
    public static extern uint GetDpiForWindow(IntPtr hwnd);

    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr hwnd, out RECT rect);

    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr hwnd, IntPtr after, int x, int y, int cx, int cy, uint flags);

    [DllImport("user32.dll")]
    public static extern bool PostMessageW(IntPtr hwnd, uint msg, IntPtr wparam, IntPtr lparam);

    public static List<MonitorEntry> ListMonitors() {
        SetThreadDpiAwarenessContext(new IntPtr(-4));
        var monitors = new List<MonitorEntry>();
        MonitorProc proc = delegate(IntPtr hMon, IntPtr hdc, ref RECT rect, IntPtr data) {
            var info = new MONITORINFOEX();
            info.cbSize = Marshal.SizeOf(info);
            if (GetMonitorInfoW(hMon, ref info)) {
                uint dpiX = 0;
                uint dpiY = 0;
                GetDpiForMonitor(hMon, 0, out dpiX, out dpiY);
                monitors.Add(new MonitorEntry {
                    Device = info.szDevice,
                    Left = info.rcMonitor.Left,
                    Top = info.rcMonitor.Top,
                    Right = info.rcMonitor.Right,
                    Bottom = info.rcMonitor.Bottom,
                    Dpi = dpiX
                });
            }
            return true;
        };
        EnumDisplayMonitors(IntPtr.Zero, IntPtr.Zero, proc, IntPtr.Zero);
        return monitors;
    }
}
"@

$monitors = [CppUiDpiNative]::ListMonitors()
$uniqueDpi = @($monitors | Select-Object -ExpandProperty Dpi -Unique)
Write-Host ("monitors={0} dpi={1}" -f $monitors.Count, ($uniqueDpi -join ','))

if ($monitors.Count -lt 2 -or $uniqueDpi.Count -lt 2) {
    Write-Host "SKIP: mixed-DPI multi-monitor setup is not available on this machine."
    exit 0
}

$process = Start-Process -FilePath $Exe -PassThru
try {
    $deadline = (Get-Date).AddSeconds(10)
    do {
        Start-Sleep -Milliseconds 50
        $process.Refresh()
    } while ($process.MainWindowHandle -eq [IntPtr]::Zero -and (Get-Date) -lt $deadline)

    $hwnd = $process.MainWindowHandle
    if ($hwnd -eq [IntPtr]::Zero) {
        throw "The demo window was not created."
    }

    foreach ($monitor in $monitors) {
        $width = [int][Math]::Round(1000.0 * $monitor.Dpi / 96.0)
        $height = [int][Math]::Round(640.0 * $monitor.Dpi / 96.0)
        $x = $monitor.Left + 40
        $y = $monitor.Top + 40

        # SWP_NOZORDER | SWP_NOACTIVATE
        if (-not [CppUiDpiNative]::SetWindowPos($hwnd, [IntPtr]::Zero, $x, $y, $width, $height, 0x0014)) {
            throw "SetWindowPos failed for $($monitor.Device)."
        }

        $dpiDeadline = (Get-Date).AddSeconds(2)
        do {
            Start-Sleep -Milliseconds 50
            $actualDpi = [CppUiDpiNative]::GetDpiForWindow($hwnd)
        } while ($actualDpi -ne $monitor.Dpi -and (Get-Date) -lt $dpiDeadline)

        if ($actualDpi -ne $monitor.Dpi) {
            throw ("DPI transition failed on {0}: expected {1}, got {2}" -f
                $monitor.Device, $monitor.Dpi, $actualDpi)
        }

        $client = New-Object CppUiDpiNative+RECT
        if (-not [CppUiDpiNative]::GetClientRect($hwnd, [ref]$client)) {
            throw "GetClientRect failed."
        }
        $clientWidth = $client.Right - $client.Left
        $clientHeight = $client.Bottom - $client.Top
        if ($clientWidth -le 0 -or $clientHeight -le 0) {
            throw "Empty client area."
        }

        Write-Host ("PASS {0}: dpi={1} client={2}x{3}" -f
            $monitor.Device, $actualDpi, $clientWidth, $clientHeight)
    }

    [void][CppUiDpiNative]::PostMessageW($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
    [void]$process.WaitForExit(5000)
}
finally {
    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
    }
}
exit 0
