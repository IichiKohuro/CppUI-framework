[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Exe,
    [Parameter(Mandatory)][string]$Configuration
)

# Runs the benchmark and enforces the Release performance budgets from
# docs/QUALITY.md. Debug builds skip the thresholds: unoptimized timings are
# not comparable, but the run itself must still succeed.

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$run = Start-Process -FilePath $Exe -ArgumentList '--benchmark' -PassThru -Wait
if ($run.ExitCode -ne 0) {
    throw "Benchmark exited with code $($run.ExitCode)."
}

$file = Join-Path (Get-Location) 'cppui-benchmark.txt'
if (-not (Test-Path -LiteralPath $file)) {
    throw "Benchmark did not write $file."
}

$metrics = @{}
foreach ($line in Get-Content -LiteralPath $file) {
    if ($line -match '^([a-z0-9_]+)=(.+)$') {
        $metrics[$matches[1]] = $matches[2].Trim()
    }
}

function Show-Metrics([string[]]$names, [hashtable]$limits) {
    $rows = foreach ($name in $names) {
        $value = if ($metrics.ContainsKey($name)) { $metrics[$name] } else { 'missing' }
        $limit = if ($limits.ContainsKey($name)) { $limits[$name] } else { '' }
        [pscustomobject]@{ Metric = $name; Value = $value; Limit = $limit }
    }
    $rows | Format-Table -AutoSize | Out-String | Write-Host
}

if (-not $metrics.ContainsKey('configuration')) {
    throw "Benchmark output has no configuration field."
}

Write-Host "CppUI benchmark gate ($Configuration, reported $($metrics['configuration']))"

if ($Configuration -ne 'Release') {
    Show-Metrics @('frame_avg_ms', 'frame_cpu_avg_ms', 'layout_avg_ms',
                   'startup_to_first_frame_ms') @{}
    Write-Host 'SKIPPED: performance thresholds apply to Release only.'
    exit 0
}

if ($metrics['configuration'] -ne 'Release') {
    throw "Configuration mismatch: expected Release, output says $($metrics['configuration'])."
}

# Deliberately generous compared with the measured Release baseline
# (CPU ~1.8 ms, layout ~0.15 ms, startup 101 ms) so that a slower clean CI
# runner or the WARP fallback does not produce false failures. They catch
# order-of-magnitude regressions, not small drift.
$limits = [ordered]@{
    frame_cpu_avg_ms          = 10.0
    frame_p95_ms              = 45.0
    layout_avg_ms             = 1.5
    resize_avg_ms             = 25.0
    startup_to_first_frame_ms = 600.0
    theme_switch_avg_ms       = 20.0
    density_switch_avg_ms     = 20.0
}

$failed = $false
$rows = foreach ($name in $limits.Keys) {
    if (-not $metrics.ContainsKey($name)) {
        Write-Error "Missing metric: $name"
        $failed = $true
        continue
    }
    $value = [double]::Parse($metrics[$name], [System.Globalization.CultureInfo]::InvariantCulture)
    $ok = $value -le $limits[$name]
    if (-not $ok) {
        $failed = $true
    }
    [pscustomobject]@{
        Metric = $name
        Value = '{0:F3}' -f $value
        Limit = '{0:F3}' -f $limits[$name]
        Status = if ($ok) { 'PASS' } else { 'FAIL' }
    }
}

$rows | Format-Table -AutoSize | Out-String | Write-Host

if ($failed) {
    throw 'Performance budget exceeded.'
}

Write-Host 'All Release performance budgets passed.'
exit 0
