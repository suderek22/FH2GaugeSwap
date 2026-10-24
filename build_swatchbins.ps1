param(
    [string]$ForzaTechDir = "E:\ForzaTech",
    [string]$AnthemArchive = ""
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $AnthemArchive) {
    $AnthemArchive = Join-Path $projectRoot "..\Anthem_1.0.0.64_x64__8wekyb3d8bbwe\Mount\media\ui\textures\Anthem.zip"
}
$AnthemArchive = [IO.Path]::GetFullPath($AnthemArchive)
$buildDirectory = Join-Path $projectRoot "build"
$stockDirectory = Join-Path $buildDirectory "stock-hud-textures"
$sourcePngDirectory = Join-Path $buildDirectory "research-textures\SourcePNGs\HUD\Speedometer"
$outputDirectory = Join-Path $buildDirectory "native-swatchbins"

if (-not (Test-Path -LiteralPath $AnthemArchive)) {
    throw "Anthem texture archive was not found: $AnthemArchive"
}
if (-not (Test-Path -LiteralPath $sourcePngDirectory)) {
    throw "Source PNGs were not found. Run .\build.ps1 first."
}

New-Item -ItemType Directory -Force -Path $stockDirectory | Out-Null
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::OpenRead($AnthemArchive)
try {
    $entries = @($archive.Entries | Where-Object {
        $_.FullName.Replace('\', '/').StartsWith(
            "HUD/Speedometer/", [StringComparison]::OrdinalIgnoreCase) -and
        $_.FullName.EndsWith(".swatchbin", [StringComparison]::OrdinalIgnoreCase)
    })
    if ($entries.Count -ne 49) {
        throw "Expected 49 stock HUD speedometer bundles in Anthem.zip; found $($entries.Count)."
    }

    foreach ($entry in $entries) {
        $destination = Join-Path $stockDirectory ([IO.Path]::GetFileName($entry.FullName))
        $inputStream = $entry.Open()
        try {
            $outputStream = [IO.File]::Create($destination)
            try {
                $inputStream.CopyTo($outputStream)
            }
            finally {
                $outputStream.Dispose()
            }
        }
        finally {
            $inputStream.Dispose()
        }
    }
}
finally {
    $archive.Dispose()
}

$converter = Join-Path $projectRoot "..\SwatchbinConverter\build.ps1"
& $converter -ForzaTechDir $ForzaTechDir `
    -StockSwatchbinDir $stockDirectory `
    -SourcePngDir $sourcePngDirectory `
    -OutputDir $outputDirectory
