[CmdletBinding()]
param(
    [string]$Output = 'samples/form_starter/assets'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$outputPath = [System.IO.Path]::GetFullPath($Output)
New-Item -ItemType Directory -Force -Path $outputPath | Out-Null

function New-RoundedPath([float]$x, [float]$y, [float]$width, [float]$height, [float]$radius) {
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $diameter = $radius * 2
    $path.AddArc($x, $y, $diameter, $diameter, 180, 90)
    $path.AddArc($x + $width - $diameter, $y, $diameter, $diameter, 270, 90)
    $path.AddArc($x + $width - $diameter, $y + $height - $diameter, $diameter, $diameter, 0, 90)
    $path.AddArc($x, $y + $height - $diameter, $diameter, $diameter, 90, 90)
    $path.CloseFigure()
    return $path
}

function Save-Mark([string]$path) {
    $bitmap = New-Object System.Drawing.Bitmap(128, 128, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
        $graphics.Clear([System.Drawing.Color]::Transparent)
        $pathData = New-RoundedPath 8 8 112 112 28
        $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
            (New-Object System.Drawing.Point(8, 8)),
            (New-Object System.Drawing.Point(120, 120)),
            [System.Drawing.Color]::FromArgb(255, 0, 112, 95),
            [System.Drawing.Color]::FromArgb(255, 0, 74, 63))
        $graphics.FillPath($brush, $pathData)
        $brush.Dispose()
        $pathData.Dispose()

        $font = New-Object System.Drawing.Font('Segoe UI', 62, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
        $textBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
        $format = New-Object System.Drawing.StringFormat
        $format.Alignment = [System.Drawing.StringAlignment]::Center
        $format.LineAlignment = [System.Drawing.StringAlignment]::Center
        $graphics.DrawString('C', $font, $textBrush, (New-Object System.Drawing.RectangleF(0, 1, 128, 128)), $format)
        $textBrush.Dispose()
        $font.Dispose()

        $dotBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(220, 111, 227, 203))
        $graphics.FillEllipse($dotBrush, 83, 29, 15, 15)
        $dotBrush.Dispose()
    }
    finally {
        $graphics.Dispose()
    }
    $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bitmap.Dispose()
}

function Save-Banner([string]$path) {
    $bitmap = New-Object System.Drawing.Bitmap(720, 260, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
        $graphics.Clear([System.Drawing.Color]::Transparent)
        $pathData = New-RoundedPath 0 0 720 260 26
        $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
            (New-Object System.Drawing.Point(0, 0)),
            (New-Object System.Drawing.Point(720, 260)),
            [System.Drawing.Color]::FromArgb(255, 0, 112, 95),
            [System.Drawing.Color]::FromArgb(255, 5, 45, 40))
        $graphics.FillPath($brush, $pathData)
        $brush.Dispose()
        $pathData.Dispose()

        $ringBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(28, 255, 255, 255))
        $graphics.FillEllipse($ringBrush, 470, -80, 360, 360)
        $graphics.FillEllipse($ringBrush, 540, 120, 260, 260)
        $ringBrush.Dispose()

        $eyebrowFont = New-Object System.Drawing.Font('Segoe UI', 15, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
        $titleFont = New-Object System.Drawing.Font('Segoe UI', 48, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
        $subtitleFont = New-Object System.Drawing.Font('Segoe UI', 16, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
        $white = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
        $soft = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(205, 255, 255, 255))
        $graphics.DrawString('SIGNAL TEAL', $eyebrowFont, $soft, 48, 38)
        $graphics.DrawString('CppUI', $titleFont, $white, 42, 72)
        $graphics.DrawString('Form Starter', $subtitleFont, $soft, 48, 141)

        $cardPath = New-RoundedPath 48 188 270 40 10
        $cardBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(38, 255, 255, 255))
        $graphics.FillPath($cardBrush, $cardPath)
        $cardBrush.Dispose()
        $cardPath.Dispose()
        $smallFont = New-Object System.Drawing.Font('Segoe UI', 14, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
        $graphics.DrawString('v0.1  |  controls / dialog / popup', $smallFont, $soft, 62, 199)

        $white.Dispose()
        $soft.Dispose()
        $eyebrowFont.Dispose()
        $titleFont.Dispose()
        $subtitleFont.Dispose()
        $smallFont.Dispose()
    }
    finally {
        $graphics.Dispose()
    }
    $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bitmap.Dispose()
}

Save-Mark (Join-Path $outputPath 'cppui-mark.png')
Save-Banner (Join-Path $outputPath 'form-banner.png')
Write-Host "Assets written to $outputPath"
