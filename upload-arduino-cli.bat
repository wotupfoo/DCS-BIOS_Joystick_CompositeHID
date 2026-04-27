@echo off
setlocal EnableExtensions
if "%1"=="" echo Provide the name of the sub-directory you want to Compile and Upload
for %%I in ("%~dp0.") do set "WORKSPACE_LIBRARY_ROOT=%%~fI"
for %%I in ("%~1") do set "SKETCH_NAME=%%~nI"
set "SKETCH_BUILD_PATH=%WORKSPACE_LIBRARY_ROOT%\.arduino-build\%SKETCH_NAME%\compile"
if "%ARDUINO_CLI%"=="" for %%P in (
    "%LocalAppData%\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "%ProgramFiles%\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "%ProgramFiles(x86)%\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
) do if not defined ARDUINO_CLI if exist "%%~P" set "ARDUINO_CLI=%%~P"
if "%ARDUINO_CLI%"=="" for /f "usebackq delims=" %%P in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "$drives = [System.IO.DriveInfo]::GetDrives() | Where-Object { $_.DriveType -eq [System.IO.DriveType]::Fixed -and $_.IsReady }; foreach ($drive in $drives) { foreach ($dir in @('Program Files', 'Program Files (x86)')) { $candidate = Join-Path $drive.Name ($dir + '\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'); if (Test-Path -LiteralPath $candidate) { Write-Output $candidate; exit 0 } } }"`) do if not defined ARDUINO_CLI set "ARDUINO_CLI=%%~P"
if exist "%LocalAppData%\Arduino15\libraries" set "ARDUINO_IDE_LIBRARIES=%LocalAppData%\Arduino15\libraries"
if "%ARDUINO_CLI%"=="" set ARDUINO_CLI=arduino-cli
if "%ARDUINO_CONFIG_FILE%"=="" set ARDUINO_CONFIG_FILE=.\arduino-cli.yaml
set "ARDUINO_CLI=%ARDUINO_CLI:"=%"
set "ARDUINO_CONFIG_FILE=%ARDUINO_CONFIG_FILE:"=%"
set "ARDUINO_IDE_LIBRARIES=%ARDUINO_IDE_LIBRARIES:"=%"
set "SKETCH_BUILD_PATH=%SKETCH_BUILD_PATH:"=%"

if "%~2"=="" (
    set "ARDUINO_FQBN="
) else (
    set "ARDUINO_FQBN=%~2"
)

echo Arduino CLI: "%ARDUINO_CLI%"
echo Arduino CLI config: "%ARDUINO_CONFIG_FILE%"
echo Sketch build path: "%SKETCH_BUILD_PATH%"
if defined ARDUINO_IDE_LIBRARIES echo Arduino IDE libraries: "%ARDUINO_IDE_LIBRARIES%"
if defined ARDUINO_FQBN (
    echo FQBN: %ARDUINO_FQBN%
) else (
    echo FQBN: sketch.yaml default_fqbn
)
echo Uploading %1
if defined ARDUINO_FQBN (
    "%ARDUINO_CLI%" --config-file "%ARDUINO_CONFIG_FILE%" upload --build-path "%SKETCH_BUILD_PATH%" --fqbn "%ARDUINO_FQBN%" --verbose "%~1"
) else (
    "%ARDUINO_CLI%" --config-file "%ARDUINO_CONFIG_FILE%" upload --build-path "%SKETCH_BUILD_PATH%" --verbose "%~1"
)
echo Done
goto :eof
