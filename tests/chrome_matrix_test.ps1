[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Exe
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

Add-Type @"
using System;
using System.Runtime.InteropServices;

public static class CppUiChromeNative {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    [DllImport("user32.dll")]
    public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);

    [DllImport("user32.dll")]
    public static extern uint GetDpiForWindow(IntPtr hwnd);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hwnd, out RECT rect);

    [DllImport("user32.dll")]
    public static extern bool IsZoomed(IntPtr hwnd);

    [DllImport("user32.dll")]
    public static extern bool IsIconic(IntPtr hwnd);

    [DllImport("user32.dll")]
    public static extern bool IsWindow(IntPtr hwnd);

    [DllImport("user32.dll")]
    public static extern IntPtr SendMessageW(IntPtr hwnd, uint msg, IntPtr wparam, IntPtr lparam);

    [DllImport("user32.dll")]
    public static extern bool PostMessageW(IntPtr hwnd, uint msg, IntPtr wparam, IntPtr lparam);

    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr hwnd, IntPtr after, int x, int y, int cx, int cy, uint flags);

    [DllImport("user32.dll")]
    public static extern bool UpdateWindow(IntPtr hwnd);

    [DllImport("dwmapi.dll")]
    public static extern int DwmFlush();
}
"@

function HitTest([IntPtr]$hwnd, [int]$x, [int]$y) {
    $lparam = [IntPtr](($y -band 0xFFFF) -shl 16 -bor ($x -band 0xFFFF))
    return [int][CppUiChromeNative]::SendMessageW($hwnd, 0x0084, [IntPtr]::Zero, $lparam)
}

function Expect([bool]$condition, [string]$message) {
    if (-not $condition) {
        throw "FAIL: $message"
    }
    Write-Host "PASS: $message"
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
    [void][CppUiChromeNative]::SetThreadDpiAwarenessContext([IntPtr](-4))
    Start-Sleep -Milliseconds 300

    $rect = New-Object CppUiChromeNative+RECT
    Expect ([CppUiChromeNative]::GetWindowRect($hwnd, [ref]$rect)) "window rect"
    $dpi = [CppUiChromeNative]::GetDpiForWindow($hwnd)
    $button = [int][Math]::Round(46.0 * $dpi / 96.0)
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top

    $caption = HitTest $hwnd ($rect.Left + 120) ($rect.Top + 20)
    Expect ($caption -eq 2) "caption hit-test (HTCAPTION)"

    $minimize = HitTest $hwnd ($rect.Right - $button * 3 + [int]($button / 2)) ($rect.Top + 20)
    Expect ($minimize -eq 8) "minimize hit-test (HTMINBUTTON)"

    $maximize = HitTest $hwnd ($rect.Right - $button * 2 + [int]($button / 2)) ($rect.Top + 20)
    Expect ($maximize -eq 9) "maximize hit-test (HTMAXBUTTON)"

    $close = HitTest $hwnd ($rect.Right - [int]($button / 2)) ($rect.Top + 20)
    Expect ($close -eq 20) "close hit-test (HTCLOSE)"

    $client = HitTest $hwnd ($rect.Left + [int]($width / 2)) ($rect.Top + [int]($height / 2))
    Expect ($client -eq 1) "client hit-test (HTCLIENT)"

    $leftBorder = HitTest $hwnd ($rect.Left + 2) ($rect.Top + [int]($height / 2))
    Expect ($leftBorder -eq 10) "left resize hit-test (HTLEFT)"

    [void][CppUiChromeNative]::SendMessageW($hwnd, 0x0112, [IntPtr]0xF030, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 300
    Expect ([CppUiChromeNative]::IsZoomed($hwnd)) "maximize"

    [void][CppUiChromeNative]::SendMessageW($hwnd, 0x0112, [IntPtr]0xF120, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 300
    Expect (-not [CppUiChromeNative]::IsZoomed($hwnd)) "restore"

    [void][CppUiChromeNative]::SendMessageW($hwnd, 0x0112, [IntPtr]0xF020, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 300
    Expect ([CppUiChromeNative]::IsIconic($hwnd)) "minimize"

    [void][CppUiChromeNative]::SendMessageW($hwnd, 0x0112, [IntPtr]0xF120, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 300
    Expect (-not [CppUiChromeNative]::IsIconic($hwnd)) "restore from minimize"

    $sizes = @(
        @{ Width = 900; Height = 580 },
        @{ Width = 1000; Height = 640 },
        @{ Width = 1120; Height = 720 },
        @{ Width = 1280; Height = 800 }
    )
    $worstFlush = 0.0
    foreach ($size in $sizes) {
        $pxWidth = [int][Math]::Round($size.Width * $dpi / 96.0)
        $pxHeight = [int][Math]::Round($size.Height * $dpi / 96.0)
        [void][CppUiChromeNative]::SetWindowPos(
            $hwnd, [IntPtr]::Zero, $rect.Left, $rect.Top, $pxWidth, $pxHeight, 0x0014)
        [void][CppUiChromeNative]::UpdateWindow($hwnd)
        $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
        [void][CppUiChromeNative]::DwmFlush()
        $stopwatch.Stop()
        $worstFlush = [Math]::Max($worstFlush, $stopwatch.Elapsed.TotalMilliseconds)
        Expect ([CppUiChromeNative]::IsWindow($hwnd)) ("responsive after " + $size.Width + "x" + $size.Height)
    }
    Write-Host ("resize DwmFlush worst: {0:F1} ms" -f $worstFlush)

    [void][CppUiChromeNative]::PostMessageW($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
    [void]$process.WaitForExit(5000)
    Expect ($process.HasExited) "close"
}
finally {
    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
    }
}
exit 0
