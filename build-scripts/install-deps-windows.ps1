# ============================================================
# CodeHex — Install & Build (Windows — PowerShell)
# Run as: powershell -ExecutionPolicy Bypass -File install-deps-windows.ps1
# ============================================================
param(
    [string]$BuildType = "Debug",
    [string]$QtVersion = "6.7.0"
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir

Write-Host "==> CodeHex Windows Setup" -ForegroundColor Cyan
Write-Host "    Project: $ProjectDir"
Write-Host ""

# ---- 1. Winget packages ----
Write-Host "==> Installing packages via winget..." -ForegroundColor Yellow
$packages = @(
    "Kitware.CMake",
    "Ninja-build.Ninja",
    "Python.Python.3.12",
    "Git.Git"
)
foreach ($pkg in $packages) {
    winget install --id $pkg --silent --accept-package-agreements --accept-source-agreements 2>$null
    if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne -1978335189) {
        Write-Warning "  Could not install $pkg (may already be installed)"
    }
}

# Refresh PATH
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" +
            [System.Environment]::GetEnvironmentVariable("Path","User")

# ---- 2. Qt via aqtinstall ----
$QtDir = "$env:USERPROFILE\Qt\${QtVersion}\msvc2019_64"
if (-not (Test-Path $QtDir)) {
    Write-Host "==> Installing Qt ${QtVersion} via aqtinstall..." -ForegroundColor Yellow
    python -m pip install aqtinstall
    python -m aqt install-qt windows desktop $QtVersion win64_msvc2019_64 `
        --modules qtmultimedia --outputdir "$env:USERPROFILE\Qt"
} else {
    Write-Host "    Qt found at $QtDir"
}

# ---- 3. Visual Studio Build Tools check ----
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    Write-Host "==> Installing Visual Studio Build Tools..." -ForegroundColor Yellow
    winget install Microsoft.VisualStudio.2022.BuildTools --silent `
        --override "--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
}

# ---- 4. Conan ----
if (-not (Get-Command conan -ErrorAction SilentlyContinue)) {
    Write-Host "==> Installing Conan..." -ForegroundColor Yellow
    python -m pip install conan
}
Write-Host "    Conan OK"
conan profile detect --force 2>$null

# ---- 5. Conan deps ----
Write-Host "==> Installing C++ dependencies via Conan ($BuildType)..." -ForegroundColor Yellow
Set-Location $ProjectDir

$OutputFolder = "build\debug\build\$BuildType"
New-Item -ItemType Directory -Force -Path $OutputFolder | Out-Null

conan install . `
    --output-folder="$OutputFolder" `
    --build=missing `
    -s build_type=$BuildType `
    -s compiler.cppstd=20 `
    -s compiler=msvc `
    -s compiler.version=193

# ---- 6. CMake configure ----
Write-Host "==> Configuring CMake..." -ForegroundColor Yellow
$QtCmake = "$QtDir\lib\cmake\Qt6"
$Toolchain = "$ProjectDir\$OutputFolder\generators\conan_toolchain.cmake"

& "cmake" -B "build\debug\cmake" `
    -G "Ninja" `
    -DCMAKE_BUILD_TYPE=$BuildType `
    "-DCMAKE_TOOLCHAIN_FILE=$Toolchain" `
    "-DCMAKE_PREFIX_PATH=$QtCmake" `
    -Wno-dev

# ---- 7. Build ----
Write-Host "==> Building CodeHex..." -ForegroundColor Yellow
$Cores = (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors
& "cmake" --build "build\debug\cmake" -j $Cores

Write-Host ""
Write-Host "==> Build complete!" -ForegroundColor Green
Write-Host "    Binary: $ProjectDir\build\debug\cmake\CodeHex.exe"
Write-Host ""
Write-Host "    To run: .\build\debug\cmake\CodeHex.exe"
