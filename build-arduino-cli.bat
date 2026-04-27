@echo off
setlocal EnableExtensions

for %%I in ("%~dp0.") do set "WORKSPACE_LIBRARY_ROOT=%%~fI"
for %%I in ("%~1") do set "SKETCH_NAME=%%~nI"
set "SKETCH_BUILD_PATH=%WORKSPACE_LIBRARY_ROOT%\.arduino-build\%SKETCH_NAME%\compile"
if "%ARDUINO_CLI%"=="" call :find_arduino_cli
call :find_arduino_ide_libraries
if "%ARDUINO_CLI%"=="" set ARDUINO_CLI=arduino-cli
if "%ARDUINO_CONFIG_FILE%"=="" set ARDUINO_CONFIG_FILE=.\arduino-cli.yaml
set "ARDUINO_CLI=%ARDUINO_CLI:"=%"
set "ARDUINO_CONFIG_FILE=%ARDUINO_CONFIG_FILE:"=%"
set "WORKSPACE_LIBRARY_ROOT=%WORKSPACE_LIBRARY_ROOT:"=%"
set "ARDUINO_IDE_LIBRARIES=%ARDUINO_IDE_LIBRARIES:"=%"
set "SKETCH_BUILD_PATH=%SKETCH_BUILD_PATH:"=%"

set "LIBRARY_ARG="
if exist "%WORKSPACE_LIBRARY_ROOT%\library.properties" (
    set "LIBRARY_ARG=--library ""%WORKSPACE_LIBRARY_ROOT%"""
)

if "%~2"=="" (
    set "ARDUINO_FQBN="
) else (
    set "ARDUINO_FQBN=%~2"
)

echo Arduino CLI: "%ARDUINO_CLI%"
echo Arduino CLI config: "%ARDUINO_CONFIG_FILE%"
echo Workspace library: "%WORKSPACE_LIBRARY_ROOT%"
echo Sketch build path: "%SKETCH_BUILD_PATH%"
if defined ARDUINO_IDE_LIBRARIES echo Arduino IDE libraries: "%ARDUINO_IDE_LIBRARIES%"
if defined ARDUINO_FQBN (
    echo FQBN: %ARDUINO_FQBN%
) else (
    echo FQBN: sketch.yaml default_fqbn
)

if defined ARDUINO_IDE_LIBRARIES (
    if defined ARDUINO_FQBN (
        "%ARDUINO_CLI%" --config-file "%ARDUINO_CONFIG_FILE%" compile --build-path "%SKETCH_BUILD_PATH%" %LIBRARY_ARG% --libraries "%ARDUINO_IDE_LIBRARIES%" --fqbn "%ARDUINO_FQBN%" --verbose "%~1"
    ) else (
        "%ARDUINO_CLI%" --config-file "%ARDUINO_CONFIG_FILE%" compile --build-path "%SKETCH_BUILD_PATH%" %LIBRARY_ARG% --libraries "%ARDUINO_IDE_LIBRARIES%" --verbose "%~1"
    )
) else (
    if defined ARDUINO_FQBN (
        "%ARDUINO_CLI%" --config-file "%ARDUINO_CONFIG_FILE%" compile --build-path "%SKETCH_BUILD_PATH%" %LIBRARY_ARG% --fqbn "%ARDUINO_FQBN%" --verbose "%~1"
    ) else (
        "%ARDUINO_CLI%" --config-file "%ARDUINO_CONFIG_FILE%" compile --build-path "%SKETCH_BUILD_PATH%" %LIBRARY_ARG% --verbose "%~1"
    )
)
if errorlevel 1 goto :compile_failed
goto :eof

:compile_failed
powershell -NoProfile -Command "Write-Host '========================================' -ForegroundColor Red; Write-Host 'COMPILE FAILED' -ForegroundColor Red; Write-Host '========================================' -ForegroundColor Red"
exit /b 1

:find_arduino_cli
for %%P in (
    "%LocalAppData%\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "%ProgramFiles%\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "%ProgramFiles(x86)%\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
) do (
    if exist "%%~P" (
        set "ARDUINO_CLI=%%~P"
        goto :eof
    )
)
for /f "usebackq delims=" %%P in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "$drives = [System.IO.DriveInfo]::GetDrives() | Where-Object { $_.DriveType -eq [System.IO.DriveType]::Fixed -and $_.IsReady }; foreach ($drive in $drives) { foreach ($dir in @('Program Files', 'Program Files (x86)')) { $candidate = Join-Path $drive.Name ($dir + '\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'); if (Test-Path -LiteralPath $candidate) { Write-Output $candidate; exit 0 } } }"`) do (
    set "ARDUINO_CLI=%%~P"
    goto :eof
)
goto :eof

:find_arduino_ide_libraries
if exist "%LocalAppData%\Arduino15\libraries" (
    set "ARDUINO_IDE_LIBRARIES=%LocalAppData%\Arduino15\libraries"
)
goto :eof
