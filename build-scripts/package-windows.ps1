# ============================================================
# CodeHex — Windows installer (.exe) via windeployqt + NSIS
# Run as: powershell -ExecutionPolicy Bypass -File package-windows.ps1
# ============================================================
param(
    [string]$QtVersion = "6.7.0",
    [string]$QtDir     = $env:QT_DIR, # Prioritize environment variable from CI
    [string]$SignCert  = ""
)

$ErrorActionPreference = "Stop"
$ScriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
$DistDir    = "$ProjectDir\dist"

# Extract version from CMakeLists.txt
$VersionLine = Select-String -Path "$ProjectDir\CMakeLists.txt" -Pattern 'project\(CodeHex VERSION (\d+\.\d+\.\d+)'
$Version = $VersionLine.Matches.Groups[1].Value
Write-Host "==> CodeHex Windows Packaging  v$Version" -ForegroundColor Cyan

if (-not $QtDir) {
    $QtDir = "$env:USERPROFILE\Qt\$QtVersion\msvc2019_64"
}
$QtBin    = "$QtDir\bin"
$BuildDir = "$ProjectDir\build\release\cmake"
$StageDir = "$ProjectDir\build\package-win"

# ---- Release build ----
Write-Host "==> Release build..." -ForegroundColor Yellow
Set-Location $ProjectDir

conan install . `
    --output-folder="build\release" `
    --build=missing `
    -s build_type=Release `
    -s compiler.cppstd=20 `
    -s compiler=msvc `
    -s compiler.version=193

cmake -B build\release\cmake `
    -G "Ninja" `
    -DCMAKE_BUILD_TYPE=Release `
    "-DCMAKE_TOOLCHAIN_FILE=$ProjectDir\build\release\build\Release\generators\conan_toolchain.cmake" `
    "-DCMAKE_PREFIX_PATH=$QtDir\lib\cmake\Qt6" `
    -Wno-dev

$Cores = (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors
cmake --build build\release\cmake -j $Cores

# ---- windeployqt ----
Write-Host "==> Running windeployqt..." -ForegroundColor Yellow
New-Item -ItemType Directory -Force -Path $StageDir | Out-Null
Copy-Item "$BuildDir\CodeHex.exe" $StageDir

& "$QtBin\windeployqt.exe" `
    --release `
    --no-translations `
    --no-opengl-sw `
    --no-system-d3d-compiler `
    "$StageDir\CodeHex.exe"

# Copy Conan runtime libs if needed (lua, etc.)
$ConanLibs = Get-ChildItem "$ProjectDir\build\release\build\Release\generators\*.dll" -ErrorAction SilentlyContinue
foreach ($lib in $ConanLibs) {
    Copy-Item $lib.FullName $StageDir
}

# ---- Code signing (optional) ----
if ($SignCert -ne "" -and (Test-Path $SignCert)) {
    Write-Host "==> Code signing..." -ForegroundColor Yellow
    $SignTool = "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe"
    & $SignTool sign /f $SignCert /t http://timestamp.digicert.com /fd SHA256 "$StageDir\CodeHex.exe"
}

# ---- NSIS installer ----
Write-Host "==> Creating NSIS installer..." -ForegroundColor Yellow
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null

$NsisScript = "$ProjectDir\build\CodeHex.nsi"
@"
!define APP_NAME "CodeHex"
!define APP_VERSION "$Version"
!define INSTALL_DIR "`$PROGRAMFILES64\CodeHex"

Name "`${APP_NAME} `${APP_VERSION}"
OutFile "$DistDir\CodeHex-$Version-windows-x64.exe"
InstallDir "`${INSTALL_DIR}"
RequestExecutionLevel admin

!include "MUI2.nsh"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Section "MainSection" SEC01
    SetOutPath "`$INSTDIR"
    File /r "$StageDir\*.*"
    CreateDirectory "`$SMPROGRAMS\CodeHex"
    CreateShortcut "`$SMPROGRAMS\CodeHex\CodeHex.lnk" "`$INSTDIR\CodeHex.exe"
    CreateShortcut "`$DESKTOP\CodeHex.lnk" "`$INSTDIR\CodeHex.exe"
    WriteUninstaller "`$INSTDIR\uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CodeHex" \
        "DisplayName" "CodeHex"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CodeHex" \
        "UninstallString" "`$INSTDIR\uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CodeHex" \
        "DisplayVersion" "$Version"
SectionEnd

Section "Uninstall"
    Delete "`$INSTDIR\*.*"
    RMDir /r "`$INSTDIR"
    Delete "`$SMPROGRAMS\CodeHex\CodeHex.lnk"
    RMDir "`$SMPROGRAMS\CodeHex"
    Delete "`$DESKTOP\CodeHex.lnk"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CodeHex"
SectionEnd
"@ | Out-File -Encoding utf8 $NsisScript

# Try to find NSIS
$MakeNsis = Get-Command makensis -ErrorAction SilentlyContinue
if (-not $MakeNsis) {
    $MakeNsis = Get-Item "C:\Program Files (x86)\NSIS\makensis.exe" -ErrorAction SilentlyContinue
}

if ($MakeNsis) {
    & $MakeNsis $NsisScript
    Write-Host ""
    Write-Host "==> Package ready: $DistDir\CodeHex-$Version-windows-x64.exe" -ForegroundColor Green
} else {
    Write-Host "WARNING: NSIS not found. Install from https://nsis.sourceforge.io/" -ForegroundColor Yellow
    Write-Host "    Staged files are in: $StageDir"
    Write-Host "    Run manually: makensis $NsisScript"
}
