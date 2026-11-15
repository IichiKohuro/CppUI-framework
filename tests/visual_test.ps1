[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Exe,
    [Parameter(Mandatory)][string]$Baseline
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.Drawing

$names = @(
    'phase5-light-cozy.png',
    'phase5-dark-cozy.png',
    'phase5-light-compact.png'
)

$capture = Start-Process -FilePath $Exe -ArgumentList '--capture-visual' -PassThru -Wait
if ($capture.ExitCode -ne 0) {
    throw "Visual capture failed with exit code $($capture.ExitCode)."
}

function Read-NormalizedBitmap([string]$path) {
    $source = [System.Drawing.Bitmap]::FromFile($path)
    try {
        $bitmap = New-Object System.Drawing.Bitmap(
            $source.Width,
            $source.Height,
            [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        try {
            $graphics.DrawImageUnscaled($source, 0, 0)
        }
        finally {
            $graphics.Dispose()
        }
        return $bitmap
    }
    finally {
        $source.Dispose()
    }
}

function Read-Pixels([System.Drawing.Bitmap]$bitmap) {
    $rect = New-Object System.Drawing.Rectangle(0, 0, $bitmap.Width, $bitmap.Height)
    $data = $bitmap.LockBits(
        $rect,
        [System.Drawing.Imaging.ImageLockMode]::ReadOnly,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $count = [Math]::Abs($data.Stride) * $bitmap.Height
        $pixels = New-Object byte[] $count
        [System.Runtime.InteropServices.Marshal]::Copy($data.Scan0, $pixels, 0, $count)
        return [pscustomobject]@{
            Bytes = $pixels
            Stride = [Math]::Abs($data.Stride)
        }
    }
    finally {
        $bitmap.UnlockBits($data)
    }
}

function Get-Sha256([string]$path) {
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        return [Convert]::ToBase64String($sha.ComputeHash([System.IO.File]::ReadAllBytes($path)))
    }
    finally {
        $sha.Dispose()
    }
}

$failed = $false
foreach ($name in $names) {
    $currentPath = Join-Path (Get-Location) (Join-Path 'visual-output' $name)
    $baselinePath = Join-Path $Baseline $name
    if (-not (Test-Path -LiteralPath $currentPath)) {
        Write-Error "Missing capture: $currentPath"
        $failed = $true
        continue
    }
    if (-not (Test-Path -LiteralPath $baselinePath)) {
        Write-Error "Missing baseline: $baselinePath"
        $failed = $true
        continue
    }

    if ((Get-Sha256 $currentPath) -eq (Get-Sha256 $baselinePath)) {
        Write-Host "PASS $name (exact)"
        continue
    }

    $current = Read-NormalizedBitmap $currentPath
    $expected = Read-NormalizedBitmap $baselinePath
    try {
        if ($current.Width -ne $expected.Width -or $current.Height -ne $expected.Height) {
            Write-Error ("Size mismatch for {0}: {1}x{2} vs {3}x{4}" -f
                $name, $current.Width, $current.Height, $expected.Width, $expected.Height)
            $failed = $true
            continue
        }

        $currentPixels = Read-Pixels $current
        $expectedPixels = Read-Pixels $expected
        $mismatch = 0
        $maxDelta = 0
        for ($y = 0; $y -lt $current.Height; $y++) {
            $row = $y * $currentPixels.Stride
            for ($x = 0; $x -lt $current.Width; $x++) {
                $offset = $row + $x * 4
                $delta = [Math]::Max(
                    [Math]::Abs([int]$currentPixels.Bytes[$offset] - [int]$expectedPixels.Bytes[$offset]),
                    [Math]::Max(
                        [Math]::Abs([int]$currentPixels.Bytes[$offset + 1] - [int]$expectedPixels.Bytes[$offset + 1]),
                        [Math]::Abs([int]$currentPixels.Bytes[$offset + 2] - [int]$expectedPixels.Bytes[$offset + 2])))
                if ($delta -gt $maxDelta) {
                    $maxDelta = $delta
                }
                if ($delta -gt 8) {
                    $mismatch++
                }
            }
        }

        $total = $current.Width * $current.Height
        $ratio = $mismatch / [double]$total
        if ($ratio -gt 0.02) {
            $diff = New-Object System.Drawing.Bitmap(
                $current.Width,
                $current.Height,
                [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
            try {
                $red = [System.Drawing.Color]::FromArgb(255, 220, 40, 40)
                $clear = [System.Drawing.Color]::FromArgb(0, 0, 0, 0)
                for ($y = 0; $y -lt $current.Height; $y++) {
                    $row = $y * $currentPixels.Stride
                    for ($x = 0; $x -lt $current.Width; $x++) {
                        $offset = $row + $x * 4
                        $delta = [Math]::Max(
                            [Math]::Abs([int]$currentPixels.Bytes[$offset] - [int]$expectedPixels.Bytes[$offset]),
                            [Math]::Max(
                                [Math]::Abs([int]$currentPixels.Bytes[$offset + 1] - [int]$expectedPixels.Bytes[$offset + 1]),
                                [Math]::Abs([int]$currentPixels.Bytes[$offset + 2] - [int]$expectedPixels.Bytes[$offset + 2])))
                        $diff.SetPixel($x, $y, $(if ($delta -gt 8) { $red } else { $clear }))
                    }
                }
                $diffPath = Join-Path (Get-Location) (Join-Path 'visual-output' ("diff-$name"))
                $diff.Save($diffPath, [System.Drawing.Imaging.ImageFormat]::Png)
                Write-Host "Diff saved: $diffPath"
            }
            finally {
                $diff.Dispose()
            }
            Write-Error ("Visual mismatch for {0}: {1}/{2} pixels ({3:P2}), max delta {4}" -f
                $name, $mismatch, $total, $ratio, $maxDelta)
            $failed = $true
        }
        else {
            Write-Host ("PASS {0}: {1}/{2} pixels differ ({3:P2}), max delta {4}" -f
                $name, $mismatch, $total, $ratio, $maxDelta)
        }
    }
    finally {
        $current.Dispose()
        $expected.Dispose()
    }
}

if ($failed) {
    exit 1
}
exit 0
