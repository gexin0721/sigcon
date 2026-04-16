param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [switch]$Run
)

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$preset = if ($Configuration -eq "Release") { "desktop-release" } else { "desktop-debug" }
$binaryDir = Join-Path $repoRoot "build\$preset\bin"
$exePath = Join-Path $binaryDir "sigcon-desktop.exe"

if (-not $env:QTDIR -and -not $env:CMAKE_PREFIX_PATH -and -not $env:Qt6_DIR) {
    Write-Warning "Qt path is not configured. Set QTDIR (recommended) or CMAKE_PREFIX_PATH before running this script."
    Write-Host 'Example: $env:QTDIR = "C:\Qt\6.8.2\mingw_64"'
}

Push-Location $repoRoot
try {
    cmake --preset $preset
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    cmake --build --preset $preset
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    if ($Run) {
        if (-not (Test-Path $exePath)) {
            Write-Error "Desktop executable not found at $exePath"
            exit 1
        }

        & $exePath
    }
}
finally {
    Pop-Location
}
