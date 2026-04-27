@echo off
setlocal EnableExtensions

if "%~1"=="" (
    echo Provide the sketch directory to compile and upload.
    exit /b 1
)

call "%~dp0build-arduino-cli.bat" %*
if errorlevel 1 goto :build_failed

call "%~dp0upload-arduino-cli.bat" %*
if errorlevel 1 goto :upload_failed

exit /b 0

:build_failed
powershell -NoProfile -Command "Write-Host '========================================' -ForegroundColor Red; Write-Host 'COMPILE FAILED - UPLOAD SKIPPED' -ForegroundColor Red; Write-Host '========================================' -ForegroundColor Red"
exit /b 1

:upload_failed
powershell -NoProfile -Command "Write-Host '========================================' -ForegroundColor Red; Write-Host 'UPLOAD FAILED' -ForegroundColor Red; Write-Host '========================================' -ForegroundColor Red"
exit /b 1
