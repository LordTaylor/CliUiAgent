@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

:: Katalog settings\ obok tego skryptu
set "SETTINGS_DIR=%~dp0settings"

:: Zbierz pliki .json z folderu settings\
set count=0
for %%f in ("%SETTINGS_DIR%\*.json") do (
    set /a count+=1
    set "FILE_!count!=%%f"
    set "LABEL_!count!=%%~nf"
)

:: Sprawdź czy są jakiekolwiek pliki
if %count%==0 (
    echo Brak plikow ustawien w %SETTINGS_DIR%
    echo Utworz plik np. lmstudio_settings.json z obiektem { "env": { ... } }
    pause
    exit /b 1
)

:: Sprawdz czy polecenie claude jest dostepne
where claude >nul 2>nul
if %errorlevel% neq 0 (
    echo Blad: Nie znaleziono polecenia 'claude'. Upewnij sie, ze Claude Code jest zainstalowany.
    pause
    exit /b 1
)

:menu
echo.
echo +==========================================+
echo ^|   Wybierz plik ustawien Claude Code      ^|
echo +==========================================+
for /l %%i in (1,1,%count%) do (
    set "label=!LABEL_%%i!                                    "
    if %%i lss 10 (
        echo ^|  %%i^) !label:~0,36! ^|
    ) else (
        echo ^| %%i^) !label:~0,36! ^|
    )
)
echo ^|  S^) Napisz skrypt Lua (nowe okno)        ^|
echo ^|  0) Wyjscie                              ^|
echo +==========================================+
echo.

set /p choice="Podaj numer [1-%count%] lub S: "

if "%choice%"=="0" (
    echo Anulowano.
    exit /b 0
)

if /I "%choice%"=="S" (
    set "LUA_DIR=%USERPROFILE%\.codehex\scripts\lua"
    if not exist "!LUA_DIR!" mkdir "!LUA_DIR!"
    set "LUA_FILE=!LUA_DIR!\nowy_skrypt.lua"
    if not exist "!LUA_FILE!" type nul > "!LUA_FILE!"
    echo Otwieram edytor w nowym oknie...
    start notepad "!LUA_FILE!"
    exit /b 0
)

set /a num=%choice% 2>nul
if !num! geq 1 if !num! leq %count% (
    set "selected=!FILE_%choice%!"
    echo.
    echo Uruchamiam: claude --settings !LABEL_%choice%!.json
    echo.
    claude --settings "!selected!"
    exit /b %errorlevel%
)

echo Nieprawidlowy wybor. Sprobuj ponownie.
goto menu
