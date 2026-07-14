param(
    [string]$PackageName = "Magpie-Experimental-x64",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$sourceRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$workspaceRoot = Split-Path $sourceRoot -Parent
$releaseRoot = [System.IO.Path]::GetFullPath((Join-Path $workspaceRoot "release"))
$stagingDir = [System.IO.Path]::GetFullPath((Join-Path $releaseRoot $PackageName))
$zipPath = [System.IO.Path]::GetFullPath((Join-Path $releaseRoot "$PackageName.zip"))
$buildOutput = Join-Path $sourceRoot "bin\x64\Release"

if (!$stagingDir.StartsWith($releaseRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
    !$zipPath.StartsWith($releaseRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Release path escaped the release directory."
}

if (!$SkipBuild) {
    $msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    $python = "C:\Users\81443\AppData\Local\Programs\Python\Python311"
    $env:Path = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;$python\Scripts;$python;" + $env:Path

    Push-Location $sourceRoot
    try {
        & $msbuild "Magpie.slnx" /m /nr:false /v:minimal /p:Configuration=Release /p:Platform=x64
        if ($LASTEXITCODE -ne 0) {
            throw "Release build failed with exit code $LASTEXITCODE."
        }
    } finally {
        Pop-Location
    }
}

if (!(Test-Path -LiteralPath (Join-Path $buildOutput "Magpie.exe"))) {
    throw "Release build output was not found: $buildOutput"
}

if (Get-Process -Name "Magpie" -ErrorAction SilentlyContinue) {
    throw "Close all running Magpie instances before updating the release directory."
}

New-Item -ItemType Directory -Path $releaseRoot -Force | Out-Null
if (Test-Path -LiteralPath $stagingDir) {
    Remove-Item -LiteralPath $stagingDir -Recurse -Force
}
New-Item -ItemType Directory -Path $stagingDir | Out-Null

Get-ChildItem -LiteralPath $buildOutput | Where-Object {
    $_.Extension -notin ".pdb", ".lib", ".exp"
} | Copy-Item -Destination $stagingDir -Recurse

Copy-Item -LiteralPath (Join-Path $sourceRoot "LICENSE") -Destination (Join-Path $stagingDir "LICENSE-Magpie.txt")
Copy-Item -LiteralPath (Join-Path $sourceRoot "docs\README-EXPERIMENTAL-RELEASE.txt") -Destination (Join-Path $stagingDir "README-Experimental.txt")

if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}

Push-Location $releaseRoot
try {
    & tar.exe -a -c -f $zipPath $PackageName
    if ($LASTEXITCODE -ne 0) {
        throw "ZIP creation failed with exit code $LASTEXITCODE."
    }
} finally {
    Pop-Location
}

$zip = Get-Item -LiteralPath $zipPath
$hash = Get-FileHash -LiteralPath $zipPath -Algorithm SHA256
Write-Host "Release directory: $stagingDir"
Write-Host "Release ZIP:       $zipPath"
Write-Host "ZIP bytes:         $($zip.Length)"
Write-Host "SHA256:            $($hash.Hash)"
