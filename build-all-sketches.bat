@echo off
setlocal EnableExtensions EnableDelayedExpansion

pushd "%~dp0" >nul || exit /b 1

set "BUILD_FQBN="
set "CONFIG_FILE=%CD%\build-all-sketches.cfg"
set "CLI_CONFIG_FILE=%ARDUINO_CONFIG_FILE%"

if "%CLI_CONFIG_FILE%"=="" (
    set "CLI_CONFIG_FILE=%CD%\arduino-cli.yaml"
)

set "CLI_CONFIG_FILE=%CLI_CONFIG_FILE:"=%"

if not "%~1"=="" (
    set "BUILD_FQBN=%~1"
)

if "%BUILD_FQBN%"=="" if exist "%CLI_CONFIG_FILE%" (
    call :read_fqbn_from_file "%CLI_CONFIG_FILE%"
)

if "%BUILD_FQBN%"=="" if exist "%CONFIG_FILE%" (
    call :read_fqbn_from_file "%CONFIG_FILE%"
)

set "BUILD_SCRIPT=%CD%\build-arduino-cli.bat"

if not exist "%BUILD_SCRIPT%" (
    echo Build helper not found: "%BUILD_SCRIPT%"
    popd
    exit /b 1
)

set /a BUILT_COUNT=0
set /a FAILED_COUNT=0

if defined BUILD_FQBN (
    echo Using FQBN: "%BUILD_FQBN%"
) else (
    echo Using sketch-local FQBNs or Arduino CLI defaults.
)

for /d %%D in (*) do (
    if exist "%%~fD\%%~nD.ino" (
        set /a BUILT_COUNT+=1
        echo(
        echo === Building %%~nD ===
        if defined BUILD_FQBN (
            call "%BUILD_SCRIPT%" "%%~fD" "!BUILD_FQBN!"
        ) else (
            call "%BUILD_SCRIPT%" "%%~fD"
        )
        if errorlevel 1 (
            set /a FAILED_COUNT+=1
            echo FAILED: %%~fD
        )
    )
)

if !BUILT_COUNT! EQU 0 (
    echo No sketch folders with matching .ino files were found in "%CD%".
    popd
    exit /b 1
)

echo(
echo Built !BUILT_COUNT! sketch(es). Failures: !FAILED_COUNT!.

popd

if !FAILED_COUNT! NEQ 0 exit /b 1
exit /b 0

:read_fqbn_from_file
for /f "usebackq tokens=1,* delims=:" %%A in ("%~1") do (
    if /I "%%~A"=="fqbn" set "BUILD_FQBN=%%~B"
)

if not "%BUILD_FQBN%"=="" goto :trim_fqbn

for /f "usebackq tokens=1,* delims==" %%A in ("%~1") do (
    if /I "%%~A"=="FQBN" set "BUILD_FQBN=%%~B"
)

if "%BUILD_FQBN%"=="" goto :eof

:trim_fqbn
for /f "tokens=* delims= " %%A in ("!BUILD_FQBN!") do set "BUILD_FQBN=%%~A"
set "BUILD_FQBN=!BUILD_FQBN:"=!"
goto :eof
