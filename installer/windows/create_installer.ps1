#
# Beatwerk — Windows Installer Builder
#
# Usage:
#   .\installer\windows\create_installer.ps1              # uses existing Release build
#   .\installer\windows\create_installer.ps1 -Build        # builds project first
#
# Output:
#   installer\output\Beatwerk-<version>-Windows.exe
#

param(
    [switch]$Build,
    [switch]$Help
)

$ErrorActionPreference = "Stop"

$ScriptDir   = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir  = (Resolve-Path "$ScriptDir\..\..").Path
$BuildDir    = "$ProjectDir\build"
$ArtefactDir = "$BuildDir\Beatwerk_artefacts\Release"
$OutputDir   = "$ProjectDir\installer\output"

if ($Help) {
    Write-Host "Usage: create_installer.ps1 [-Build] [-Help]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -Build   Build project in Release mode before creating installer"
    Write-Host "  -Help    Show this help"
    exit 0
}

# Read version from CMakeLists.txt
$CmakeContent = Get-Content "$ProjectDir\CMakeLists.txt" -Raw
if ($CmakeContent -match 'project\(Beatwerk\s+VERSION\s+(\d+\.\d+\.\d+)') {
    $Version = $Matches[1]
} else {
    $Version = "1.0.0"
    Write-Warning "Could not read version from CMakeLists.txt, using $Version"
}

Write-Host "============================================"
Write-Host "  Beatwerk — Windows Installer Builder"
Write-Host "  Version: $Version"
Write-Host "============================================"
Write-Host ""

# Step 1: Optionally build the project
if ($Build) {
    Write-Host "[1/4] Building project in Release mode..."
    cmake -B $BuildDir -G "Visual Studio 17 2022" -A x64 $ProjectDir
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

    cmake --build $BuildDir --config Release -j $env:NUMBER_OF_PROCESSORS
    if ($LASTEXITCODE -ne 0) { throw "CMake build failed" }

    Write-Host "       Build complete."
} else {
    Write-Host "[1/4] Skipping build (use -Build to compile first)"
}

# Step 2: Verify artefacts
Write-Host "[2/4] Verifying build artefacts..."

$StandaloneSrc = "$ArtefactDir\Standalone\Beatwerk.exe"
$Vst3Src       = "$ArtefactDir\VST3\Beatwerk.vst3"

$Missing = $false
foreach ($artefact in @($StandaloneSrc, $Vst3Src)) {
    if (-not (Test-Path $artefact)) {
        Write-Host "       ERROR: Missing artefact: $artefact"
        $Missing = $true
    }
}

if ($Missing) {
    Write-Host ""
    Write-Host "Build artefacts not found. Run with -Build flag or build manually first:"
    Write-Host '  cmake -B build -G "Visual Studio 17 2022" -A x64'
    Write-Host "  cmake --build build --config Release"
    exit 1
}
Write-Host "       All artefacts found."

# Step 3: Build installer with Inno Setup
Write-Host "[3/4] Creating installer..."

if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

$IssFile = "$ScriptDir\beatwerk.iss"

$IsccPaths = @(
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
    "C:\Program Files\Inno Setup 6\ISCC.exe"
)

$Iscc = $null
foreach ($p in $IsccPaths) {
    if (Test-Path $p) { $Iscc = $p; break }
}

if (-not $Iscc) {
    $IsccFromPath = Get-Command iscc.exe -ErrorAction SilentlyContinue
    if ($IsccFromPath) { $Iscc = $IsccFromPath.Source }
}

if (-not $Iscc) {
    throw "Inno Setup not found. Install from https://jrsoftware.org/isinfo.php"
}

& $Iscc $IssFile "/DAppVersion=$Version"
if ($LASTEXITCODE -ne 0) { throw "Inno Setup compilation failed" }

Write-Host "       Installer created."

# Step 4: Summary
$InstallerExe = Get-ChildItem "$OutputDir\Beatwerk-*-Windows.exe" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
$InstallerSize = "{0:N1} MB" -f ($InstallerExe.Length / 1MB)

Write-Host ""
Write-Host "============================================"
Write-Host "  Installer created successfully!"
Write-Host ""
Write-Host "  File: $($InstallerExe.FullName)"
Write-Host "  Size: $InstallerSize"
Write-Host "============================================"
Write-Host ""
Write-Host "The installer will install:"
Write-Host "  - Standalone App  ->  C:\Program Files\Beatwerk\"
Write-Host "  - VST3 Plugin     ->  C:\Program Files\Common Files\VST3\Beatwerk.vst3\"
Write-Host ""
