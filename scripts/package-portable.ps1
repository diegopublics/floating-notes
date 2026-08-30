param(
    [string]$BuildDir = "build-release",
    [string]$OutputDir = "dist/FloatingNotes",
    [string]$QtPrefix = ""
)

$ErrorActionPreference = "Stop"

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildPath = Join-Path $projectRoot $BuildDir
$outputPath = Join-Path $projectRoot $OutputDir

if (Test-Path -LiteralPath $outputPath) {
    throw "Portable output already exists: $outputPath. Remove it before packaging again."
}

$configureArgs = @(
    "-S", $projectRoot,
    "-B", $buildPath,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=Release"
)

if (-not [string]::IsNullOrWhiteSpace($QtPrefix)) {
    $configureArgs += "-DCMAKE_PREFIX_PATH=$QtPrefix"
}

& cmake @configureArgs
& cmake --build $buildPath

$executable = Join-Path $buildPath "FloatingNotes.exe"
if (-not (Test-Path -LiteralPath $executable)) {
    throw "Build did not produce: $executable"
}

if (-not [string]::IsNullOrWhiteSpace($QtPrefix)) {
    $windeployqt = Join-Path $QtPrefix "bin/windeployqt.exe"
} else {
    $windeployqt = (Get-Command windeployqt.exe -ErrorAction Stop).Source
}

if (-not (Test-Path -LiteralPath $windeployqt)) {
    throw "windeployqt was not found: $windeployqt"
}

New-Item -ItemType Directory -Path $outputPath | Out-Null
$packagedExecutable = Join-Path $outputPath "FloatingNotes.exe"
Copy-Item -LiteralPath $executable -Destination $packagedExecutable

& $windeployqt --release --compiler-runtime --no-translations --include-plugins qsqlite $packagedExecutable

Write-Output "Portable package created at: $outputPath"
