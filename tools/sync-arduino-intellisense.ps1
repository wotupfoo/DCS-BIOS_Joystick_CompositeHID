param(
    [string]$WorkspaceRoot = (Join-Path $PSScriptRoot ".."),
    [string]$Sketch,
    [string]$Fqbn
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-FullPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [string]$BasePath
    )

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    if ([string]::IsNullOrWhiteSpace($BasePath)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $BasePath $Path))
}

function Convert-ToJsonPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [string]$WorkspaceRoot
    )

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $fullWorkspaceRoot = [System.IO.Path]::GetFullPath($WorkspaceRoot)

    if ($fullPath.StartsWith($fullWorkspaceRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relativePath = $fullPath.Substring($fullWorkspaceRoot.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relativePath)) {
            return '${workspaceFolder}'
        }

        return ('${workspaceFolder}/' + $relativePath.Replace('\', '/'))
    }

    return $fullPath.Replace('\', '/')
}

function Add-OrderedValue {
    param(
        [Parameter(Mandatory = $true)]
        [System.Collections.IDictionary]$Map,
        [string]$Value
    )

    if (-not [string]::IsNullOrWhiteSpace($Value) -and -not $Map.Contains($Value)) {
        $Map[$Value] = $true
    }
}

function Find-ArduinoCli {
    $candidates = @()

    if ($env:ARDUINO_CLI) {
        $candidates += $env:ARDUINO_CLI.Trim('"')
    }

    $candidates += @(
        "$env:LocalAppData\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe",
        "$env:ProgramFiles\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe",
        "${env:ProgramFiles(x86)}\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    )

    foreach ($candidate in $candidates) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and (Test-Path -LiteralPath $candidate)) {
            return [System.IO.Path]::GetFullPath($candidate)
        }
    }

    $fixedDrives = @([System.IO.DriveInfo]::GetDrives() | Where-Object { $_.DriveType -eq [System.IO.DriveType]::Fixed -and $_.IsReady })
    foreach ($drive in $fixedDrives) {
        $driveRoot = $drive.Name
        foreach ($programFilesDir in @("Program Files", "Program Files (x86)")) {
            $candidate = Join-Path $driveRoot ($programFilesDir + "\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe")
            if (Test-Path -LiteralPath $candidate) {
                return [System.IO.Path]::GetFullPath($candidate)
            }
        }
    }

    $command = Get-Command arduino-cli -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    throw "Unable to find arduino-cli.exe. Set ARDUINO_CLI or install Arduino IDE."
}

function Get-ArduinoConfig {
    param([string]$ArduinoJsonPath)

    if (-not (Test-Path -LiteralPath $ArduinoJsonPath)) {
        return $null
    }

    return Get-Content -LiteralPath $ArduinoJsonPath -Raw | ConvertFrom-Json
}

function Resolve-SketchDirectory {
    param(
        [string]$WorkspaceRoot,
        $ArduinoConfig,
        [string]$SketchOverride
    )

    $sketchCandidate = $null

    if (-not [string]::IsNullOrWhiteSpace($SketchOverride)) {
        $sketchCandidate = Get-FullPath -Path $SketchOverride -BasePath $WorkspaceRoot
    } elseif ($ArduinoConfig -and $ArduinoConfig.PSObject.Properties.Name -contains 'sketch' -and -not [string]::IsNullOrWhiteSpace($ArduinoConfig.sketch)) {
        $sketchCandidate = Get-FullPath -Path ([string]$ArduinoConfig.sketch) -BasePath $WorkspaceRoot
    } else {
        $workspaceName = Split-Path -Path $WorkspaceRoot -Leaf
        $defaultSketchFile = Join-Path (Join-Path $WorkspaceRoot $workspaceName) ($workspaceName + ".ino")
        if (Test-Path -LiteralPath $defaultSketchFile) {
            $sketchCandidate = $defaultSketchFile
        }
    }

    if ([string]::IsNullOrWhiteSpace($sketchCandidate)) {
        $sketchFiles = @(Get-ChildItem -Path $WorkspaceRoot -Recurse -Filter *.ino -File | Where-Object { $_.FullName -notlike "*\.arduino-build\*" })
        if ($sketchFiles.Count -eq 1) {
            $sketchCandidate = $sketchFiles[0].FullName
        } else {
            throw "Unable to determine which sketch to analyze. Set .vscode/arduino.json 'sketch' or pass -Sketch."
        }
    }

    if (Test-Path -LiteralPath $sketchCandidate -PathType Container) {
        return [System.IO.Path]::GetFullPath($sketchCandidate)
    }

    if (Test-Path -LiteralPath $sketchCandidate -PathType Leaf) {
        return [System.IO.Path]::GetFullPath((Split-Path -Path $sketchCandidate -Parent))
    }

    throw "Sketch path not found: $sketchCandidate"
}

function Get-FqbnFromSketchYaml {
    param([string]$SketchDirectory)

    $sketchYamlPath = Join-Path $SketchDirectory "sketch.yaml"
    if (-not (Test-Path -LiteralPath $sketchYamlPath)) {
        return $null
    }

    foreach ($line in Get-Content -LiteralPath $sketchYamlPath) {
        if ($line -match '^\s*default_fqbn\s*:\s*(.+?)\s*$') {
            return $matches[1].Trim().Trim('"')
        }
    }

    return $null
}

function Resolve-Fqbn {
    param(
        $ArduinoConfig,
        [string]$SketchDirectory,
        [string]$FqbnOverride
    )

    if (-not [string]::IsNullOrWhiteSpace($FqbnOverride)) {
        return $FqbnOverride
    }

    if ($ArduinoConfig) {
        if ($ArduinoConfig.PSObject.Properties.Name -contains 'fqbn' -and -not [string]::IsNullOrWhiteSpace($ArduinoConfig.fqbn)) {
            return [string]$ArduinoConfig.fqbn
        }

        if ($ArduinoConfig.PSObject.Properties.Name -contains 'board' -and -not [string]::IsNullOrWhiteSpace($ArduinoConfig.board)) {
            $resolvedBoard = [string]$ArduinoConfig.board
            $resolvedConfiguration = ""

            if ($ArduinoConfig.PSObject.Properties.Name -contains 'configuration' -and -not [string]::IsNullOrWhiteSpace($ArduinoConfig.configuration)) {
                $resolvedConfiguration = [string]$ArduinoConfig.configuration
            }

            if ([string]::IsNullOrWhiteSpace($resolvedConfiguration)) {
                return $resolvedBoard
            }

            return ($resolvedBoard + ":" + $resolvedConfiguration)
        }
    }

    return Get-FqbnFromSketchYaml -SketchDirectory $SketchDirectory
}

$resolvedWorkspaceRoot = [System.IO.Path]::GetFullPath($WorkspaceRoot)
$arduinoJsonPath = Join-Path $resolvedWorkspaceRoot ".vscode\arduino.json"
$cppPropertiesPath = Join-Path $resolvedWorkspaceRoot ".vscode\c_cpp_properties.json"
$arduinoConfig = Get-ArduinoConfig -ArduinoJsonPath $arduinoJsonPath
$sketchDirectory = Resolve-SketchDirectory -WorkspaceRoot $resolvedWorkspaceRoot -ArduinoConfig $arduinoConfig -SketchOverride $Sketch
$resolvedFqbn = Resolve-Fqbn -ArduinoConfig $arduinoConfig -SketchDirectory $sketchDirectory -FqbnOverride $Fqbn
$sketchName = Split-Path -Path $sketchDirectory -Leaf

$buildPath = Join-Path (Join-Path (Join-Path $resolvedWorkspaceRoot ".arduino-build") $sketchName) "intellisense"
if ($arduinoConfig -and $arduinoConfig.PSObject.Properties.Name -contains 'output' -and -not [string]::IsNullOrWhiteSpace($arduinoConfig.output)) {
    $buildPath = Get-FullPath -Path ([string]$arduinoConfig.output) -BasePath $resolvedWorkspaceRoot
}

New-Item -ItemType Directory -Force -Path (Split-Path -Path $cppPropertiesPath -Parent) | Out-Null
New-Item -ItemType Directory -Force -Path $buildPath | Out-Null

$arduinoCli = Find-ArduinoCli
$cliConfigFile = $null

if (-not [string]::IsNullOrWhiteSpace($env:ARDUINO_CONFIG_FILE)) {
    $cliConfigFile = Get-FullPath -Path $env:ARDUINO_CONFIG_FILE.Trim('"') -BasePath $resolvedWorkspaceRoot
} else {
    $workspaceConfigFile = Join-Path $resolvedWorkspaceRoot "arduino-cli.yaml"
    if (Test-Path -LiteralPath $workspaceConfigFile) {
        $cliConfigFile = $workspaceConfigFile
    }
}

$arduinoIdeLibraries = Join-Path $env:LocalAppData "Arduino15\libraries"
$cliArgs = @()

if ($cliConfigFile) {
    $cliArgs += @("--config-file", $cliConfigFile)
}

$cliArgs += @(
    "compile",
    "--only-compilation-database",
    "--build-path", $buildPath,
    "--library", $resolvedWorkspaceRoot
)

if (Test-Path -LiteralPath $arduinoIdeLibraries) {
    $cliArgs += @("--libraries", $arduinoIdeLibraries)
}

if (-not [string]::IsNullOrWhiteSpace($resolvedFqbn)) {
    $cliArgs += @("--fqbn", $resolvedFqbn)
}

$cliArgs += $sketchDirectory

Write-Host ("Arduino CLI: " + $arduinoCli)
if ($cliConfigFile) {
    Write-Host ("Arduino CLI config: " + $cliConfigFile)
}
if (-not [string]::IsNullOrWhiteSpace($resolvedFqbn)) {
    Write-Host ("FQBN: " + $resolvedFqbn)
} else {
    Write-Host "FQBN: using sketch.yaml or profile defaults"
}
Write-Host ("Sketch: " + $sketchDirectory)

& $arduinoCli @cliArgs
if ($LASTEXITCODE -ne 0) {
    throw "arduino-cli failed while generating the compilation database."
}

$compileCommandsPath = Join-Path $buildPath "compile_commands.json"
if (-not (Test-Path -LiteralPath $compileCommandsPath)) {
    throw "Compilation database not found at $compileCommandsPath"
}

$compileCommands = Get-Content -LiteralPath $compileCommandsPath -Raw | ConvertFrom-Json
if (-not $compileCommands -or $compileCommands.Count -eq 0) {
    throw "Compilation database is empty."
}

$cppEntry = $compileCommands | Where-Object { $_.arguments[0] -match 'g\+\+(\.exe)?$' } | Select-Object -First 1
$cEntry = $compileCommands | Where-Object { $_.arguments[0] -match '(^|\\)gcc(\.exe)?$' } | Select-Object -First 1
if (-not $cppEntry) {
    $cppEntry = $compileCommands | Select-Object -First 1
}

$compilerPath = [string]$cppEntry.arguments[0]
$includePathMap = [ordered]@{}
$defineMap = [ordered]@{}
$compilerArgMap = [ordered]@{}
$cppStandard = $null
$cStandard = $null

foreach ($entry in $compileCommands) {
    $arguments = @($entry.arguments)
    for ($i = 0; $i -lt $arguments.Count; $i++) {
        $argument = [string]$arguments[$i]

        if ($argument -eq "-I" -and ($i + 1) -lt $arguments.Count) {
            $i++
            Add-OrderedValue -Map $includePathMap -Value (([string]$arguments[$i]).Replace('\', '/'))
            continue
        }

        if ($argument.StartsWith("-I") -and $argument.Length -gt 2) {
            Add-OrderedValue -Map $includePathMap -Value ($argument.Substring(2).Replace('\', '/'))
            continue
        }

        if ($argument -eq "-D" -and ($i + 1) -lt $arguments.Count) {
            $i++
            Add-OrderedValue -Map $defineMap -Value ([string]$arguments[$i])
            continue
        }

        if ($argument.StartsWith("-D") -and $argument.Length -gt 2) {
            Add-OrderedValue -Map $defineMap -Value $argument.Substring(2)
            continue
        }

        if ($argument.StartsWith("-std=")) {
            $standard = $argument.Substring(5)
            if ($arguments[0] -match 'g\+\+(\.exe)?$') {
                $cppStandard = $standard
            } elseif (-not $cStandard) {
                $cStandard = $standard
            }
            continue
        }

        if ($argument -match '^-(mcpu|march|mthumb|mfpu|mfloat-abi)') {
            Add-OrderedValue -Map $compilerArgMap -Value $argument
            continue
        }

        if ($argument.StartsWith("--target=") -or $argument.StartsWith("--sysroot=")) {
            Add-OrderedValue -Map $compilerArgMap -Value $argument
            continue
        }
    }
}

if (-not $cStandard -and $cEntry) {
    foreach ($argument in @($cEntry.arguments)) {
        if ([string]$argument -like "-std=*") {
            $cStandard = ([string]$argument).Substring(5)
            break
        }
    }
}

$configuration = [ordered]@{
    name = "Arduino"
    compilerPath = $compilerPath.Replace('\', '/')
    compileCommands = Convert-ToJsonPath -Path $compileCommandsPath -WorkspaceRoot $resolvedWorkspaceRoot
    includePath = @('${workspaceFolder}/**') + @($includePathMap.Keys)
    defines = @($defineMap.Keys)
}

if ($compilerArgMap.Count -gt 0) {
    $configuration.compilerArgs = @($compilerArgMap.Keys)
}

if (-not [string]::IsNullOrWhiteSpace($cStandard)) {
    $configuration.cStandard = $cStandard
}

if (-not [string]::IsNullOrWhiteSpace($cppStandard)) {
    $configuration.cppStandard = $cppStandard
}

$jsonObject = [ordered]@{
    configurations = @($configuration)
    version = 4
}

$jsonText = $jsonObject | ConvertTo-Json -Depth 8
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($cppPropertiesPath, ($jsonText + [System.Environment]::NewLine), $utf8NoBom)

Write-Host ("Updated " + $cppPropertiesPath)
