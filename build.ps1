$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDirectory = Join-Path $projectRoot "build"
$researchDirectory = Join-Path $buildDirectory "research-textures"
$distributionDirectory = Join-Path $projectRoot "dist"
$textureBuilder = Join-Path $projectRoot "tools\build_texture.py"
$settingsTemplate = Join-Path $projectRoot "ForzaGaugeSwap.ini"
$layoutSource = Join-Path $projectRoot "UI\Controls\HUDSpeedometerControl.xaml"
$cmakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue
$cmakePath = $null

New-Item -ItemType Directory -Force -Path $distributionDirectory | Out-Null

python $textureBuilder --output-dir $researchDirectory
if ($LASTEXITCODE -ne 0) {
    throw "Gauge source image generation failed with exit code $LASTEXITCODE."
}

python -m unittest discover -s (Join-Path $projectRoot "tests")
if ($LASTEXITCODE -ne 0) {
    throw "Gauge texture validation failed with exit code $LASTEXITCODE."
}

if ($cmakeCommand) {
    $cmakePath = $cmakeCommand.Source
}
else {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $installationPath = & $vswhere -latest -products "*" -property installationPath
        $candidate = Join-Path $installationPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        if (Test-Path $candidate) {
            $cmakePath = $candidate
        }
    }
}

if (-not $cmakePath) {
    throw "CMake was not found. Install CMake or the Visual Studio CMake component."
}

& $cmakePath -S $projectRoot -B $buildDirectory -A x64
if ($LASTEXITCODE -ne 0) {
    throw "CMake configuration failed with exit code $LASTEXITCODE."
}

& $cmakePath --build $buildDirectory --config Release
if ($LASTEXITCODE -ne 0) {
    throw "Mod compilation failed with exit code $LASTEXITCODE."
}

Copy-Item -LiteralPath $settingsTemplate `
    -Destination (Join-Path $distributionDirectory "ForzaGaugeSwap.ini") -Force
$layoutDistributionPath = Join-Path $distributionDirectory "UI\Controls"
New-Item -ItemType Directory -Force -Path $layoutDistributionPath | Out-Null
Copy-Item -LiteralPath $layoutSource `
    -Destination (Join-Path $layoutDistributionPath "HUDSpeedometerControl.xaml") -Force

Write-Host "Build complete. Copy the contents of '$distributionDirectory' into fh2-mods."
