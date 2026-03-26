@echo off
if "%1"=="" echo Provide the name of the sub-directory you want to Compile and Upload
if "%ARDUINO_CLI%"=="" set ARDUINO_CLI=arduino-cli
if "%ARDUINO_CONFIG_FILE%"=="" set ARDUINO_CONFIG_FILE=".\arduino-cli.yaml"

rem handling the FQBN in firmware\sketch.yaml
rem %ARDUINO_CLI% --config-file %ARDUINO_CONFIG_FILE% compile --fqbn Arduino_STM32:STM32F1:genericSTM32F103C:upload_method=STLinkMethod firmware
echo Uploading %1
%ARDUINO_CLI% --config-file %ARDUINO_CONFIG_FILE% upload --verbose %1
echo Done
