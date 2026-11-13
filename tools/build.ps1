[CmdletBinding()]
param(
    [ValidateSet(
        'windows-vs2026-debug',
        'windows-vs2026-release',
        'windows-vs2022-debug',
        'windows-vs2022-release',
        'windows-msvc-debug',
        'windows-msvc-release'
    )]
    [string]$Preset = 'windows-vs2026-debug',

    [switch]$Test,

    [switch]$ConfigureOnly
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$root = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'

if (-not (Test-Path -LiteralPath $vswhere)) {
    throw "vswhere.exe was not found. Install Visual Studio with the C++ workload."
}

$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ([string]::IsNullOrWhiteSpace($vsPath)) {
    throw "Visual Studio with the MSVC x64 toolchain was not found."
}

$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
$cmake = Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ninja = Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'

if (-not (Test-Path -LiteralPath $vcvars)) {
    throw "vcvars64.bat was not found at $vcvars"
}
if (-not (Test-Path -LiteralPath $cmake)) {
    $cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
    if ($null -eq $cmakeCommand) {
        throw "cmake.exe was not found. Install the CMake component or add CMake to PATH."
    }
    $cmake = $cmakeCommand.Source
}
if (-not (Test-Path -LiteralPath $ninja)) {
    $ninjaCommand = Get-Command ninja -ErrorAction SilentlyContinue
    if ($null -eq $ninjaCommand) {
        throw "ninja.exe was not found. Install the CMake component or add Ninja to PATH."
    }
    $ninja = $ninjaCommand.Source
}

$cmakeDir = Split-Path -Parent $cmake
$ninjaDir = Split-Path -Parent $ninja

# Put the CMake/Ninja components on PATH before vcvars64.bat runs. cmd expands
# %PATH% before executing the compound command, so appending after `call`
# would not see the compiler paths.
$buildCommand = 'set "PATH={0};{1};%PATH%" && call "{2}" >nul && "{3}" --preset {4}' -f $cmakeDir, $ninjaDir, $vcvars, $cmake, $Preset
if (-not $ConfigureOnly) {
    $buildCommand += ' && "{0}" --build --preset {1}' -f $cmake, $Preset
}

Push-Location $root
try {
    Write-Host "CppUI :: $Preset"
    & cmd.exe /d /s /c $buildCommand
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    if ($Test -and -not $ConfigureOnly) {
        $ctest = Join-Path $cmakeDir 'ctest.exe'
        if (-not (Test-Path -LiteralPath $ctest)) {
            throw "ctest.exe was not found at $ctest"
        }
        & $ctest --preset $Preset
        exit $LASTEXITCODE
    }
}
finally {
    Pop-Location
}

exit 0
