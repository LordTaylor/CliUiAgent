# ============================================================
# CodeHex — build (if needed) and run  [Windows / PowerShell]
# Usage:
#   .\build-scripts\run.ps1              # debug build (default)
#   .\build-scripts\run.ps1 -Release     # release build
#   .\build-scripts\run.ps1 -Rebuild     # force full rebuild first
#   .\build-scripts\run.ps1 -Release -Rebuild
# ============================================================
param(
    [switch]$Release,
    [switch]$Rebuild,
    [string]$QtVersion = "6.7.0"
)

$ErrorActionPreference = "Stop"
$ScriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir

$BuildType = if ($Release) { "release" } else { "debug" }
$BuildUpper = if ($Release) { "Release" } else { "Debug" }
$BuildDir  = "$ProjectDir\build\$BuildType"
$CmakeDir  = "$BuildDir\cmake"
$QtDir     = "$env:USERPROFILE\Qt\$QtVersion\msvc2019_64"
$Bin       = "$CmakeDir\CodeHex.exe"

Write-Host "==> CodeHex Run  [$BuildType]" -ForegroundColor Cyan
Set-Location $ProjectDir

# ---- Rebuild if requested or binary missing ----
$NeedsBuild = $false

if ($Rebuild) {
    Write-Host "==> --Rebuild requested, removing $CmakeDir ..." -ForegroundColor Yellow
    if (Test-Path $CmakeDir) { Remove-Item -Recurse -Force $CmakeDir }
    $NeedsBuild = $true
} elseif (-not (Test-Path $Bin)) {
    Write-Host "==> Binary not found: $Bin" -ForegroundColor Yellow
    $NeedsBuild = $true
}

if ($NeedsBuild) {
    Write-Host "==> Conan install ($BuildType)..." -ForegroundColor Yellow
    conan install . `
        --output-folder="$BuildDir\build\$BuildUpper" `
        --build=missing `
        -s build_type=$BuildUpper `
        -s compiler.cppstd=20 `
        -s compiler=msvc `
        -s compiler.version=193

    Write-Host "==> CMake configure..." -ForegroundColor Yellow
    cmake -B $CmakeDir `
        -G "Ninja" `
        -DCMAKE_BUILD_TYPE=$BuildUpper `
        "-DCMAKE_TOOLCHAIN_FILE=$BuildDir\build\$BuildUpper\generators\conan_toolchain.cmake" `
        "-DCMAKE_PREFIX_PATH=$QtDir\lib\cmake\Qt6" `
        -Wno-dev

    $Cores = (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors
    Write-Host "==> Building ($Cores cores)..." -ForegroundColor Yellow
    cmake --build $CmakeDir -j $Cores
}

# ---- Verify ----
if (-not (Test-Path $Bin)) {
    Write-Host "ERROR: Build succeeded but binary not found at: $Bin" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "==> Launching CodeHex ($BuildType) ..." -ForegroundColor Green
Write-Host "    $Bin"
Write-Host ""

Start-Process -FilePath $Bin
