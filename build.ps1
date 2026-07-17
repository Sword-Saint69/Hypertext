# build.ps1 — HyperCore build helper for Windows
# Runs cmake/build/test inside the VS Build Tools environment automatically.
#
# Usage:
#   .\build.ps1              — configure + build + test (Release)
#   .\build.ps1 -Config Debug
#   .\build.ps1 -SkipTests
#   .\build.ps1 -Run         — also runs `htx --help` after build

param(
    [string] $Config     = "Release",
    [switch] $SkipTests  = $false,
    [switch] $Run        = $false,
    [switch] $Clean      = $false
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ─── Find VS Build Tools environment ─────────────────────────────────────────
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vsWhere)) {
    Write-Error "vswhere.exe not found. Is Visual Studio Build Tools installed?"
    exit 1
}

$vsInstallPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsInstallPath) {
    Write-Error "No Visual Studio installation with C++ tools found."
    exit 1
}

Write-Host "VS Install: $vsInstallPath" -ForegroundColor Cyan

# ─── Locate cmake, ninja, cl ─────────────────────────────────────────────────
$cmakeBin  = "$vsInstallPath\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
$ninjaBin  = "$vsInstallPath\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"

if (-not (Test-Path "$cmakeBin\cmake.exe")) {
    Write-Error "cmake.exe not found in: $cmakeBin`nInstall CMake component via Visual Studio Installer."
    exit 1
}

# ─── Set up MSVC environment via vcvars64.bat ─────────────────────────────────
$vcvars = "$vsInstallPath\VC\Auxiliary\Build\vcvars64.bat"

# Capture env vars set by vcvars64
$envDump = cmd /c "`"$vcvars`" && set" 2>&1
foreach ($line in $envDump) {
    if ($line -match "^([^=]+)=(.*)$") {
        [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
    }
}

# Add cmake + ninja to PATH for this session
$env:PATH = "$cmakeBin;$ninjaBin;" + $env:PATH

Write-Host ""
Write-Host "══════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "  HyperCore Build ($Config)" -ForegroundColor Cyan
Write-Host "══════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "  cmake  : $(cmake --version | Select-String 'cmake version')"
Write-Host "  ninja  : $(ninja --version)"
try { Write-Host "  cl     : $(& cl.exe 2>&1 | Select-Object -First 1)" } catch { Write-Host "  cl     : (found but stderr blocked)" }
Write-Host ""

# ─── Clean ────────────────────────────────────────────────────────────────────
$buildDir = "build\$($Config.ToLower())"
if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "[clean] Removing $buildDir ..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}

# ─── Configure ────────────────────────────────────────────────────────────────
Write-Host "[cmake] Configuring ($Config)..." -ForegroundColor Green
$preset = $Config.ToLower()

cmake --preset $preset
if ($LASTEXITCODE -ne 0) {
    Write-Error "cmake configure failed (exit $LASTEXITCODE)"
    exit $LASTEXITCODE
}

# ─── Build ────────────────────────────────────────────────────────────────────
Write-Host ""
Write-Host "[build] Building..." -ForegroundColor Green
cmake --build $buildDir --parallel
if ($LASTEXITCODE -ne 0) {
    Write-Error "cmake build failed (exit $LASTEXITCODE)"
    exit $LASTEXITCODE
}

# ─── Test ─────────────────────────────────────────────────────────────────────
if (-not $SkipTests) {
    Write-Host ""
    Write-Host "[test] Running tests..." -ForegroundColor Green
    ctest --test-dir $buildDir --output-on-failure --parallel 4
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Tests failed (exit $LASTEXITCODE)"
        exit $LASTEXITCODE
    }
    Write-Host ""
    Write-Host "All tests passed!" -ForegroundColor Green
}

# ─── Run htx --help ───────────────────────────────────────────────────────────
if ($Run) {
    Write-Host ""
    Write-Host "[run] htx --help" -ForegroundColor Green
    & ".\$buildDir\src\cli\htx.exe" --help
}

Write-Host ""
Write-Host "══════════════════════════════════════════" -ForegroundColor Green
Write-Host "  Build complete: .\$buildDir\htx.exe" -ForegroundColor Green
Write-Host "══════════════════════════════════════════" -ForegroundColor Green
