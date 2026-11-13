#requires -Version 5.1
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location -LiteralPath $root

$tcc = Join-Path $root 'tools\tcc\tcc\tcc.exe'
$rc  = Join-Path $root 'tools\rcedit.exe'
$out = Join-Path $root 'NovolineLauncher.exe'

if (-not (Test-Path -LiteralPath $tcc)) { throw "TinyCC not found: $tcc" }
if (-not (Test-Path -LiteralPath $rc))  { throw "rcedit not found: $rc" }

$sources = @(
    'src\main.c',
    'src\json.c',
    'src\launcher.c',
    'src\versions.c'
)
$libs = @(
    '-lgdiplus',
    '-lshell32',
    '-luser32',
    '-lgdi32',
    '-ladvapi32',
    '-lwinhttp',
    '-lkernel32'
)

& $tcc -m64 -O2 -I src -o $out @sources @libs '-Wl,-subsystem=windows'
if ($LASTEXITCODE -ne 0) { throw "TinyCC failed with exit code $LASTEXITCODE" }

& $rc $out `
    --set-icon 'assets\novoline.ico' `
    --application-manifest 'assets\app.manifest' `
    --set-file-version '1.0.0.0' `
    --set-product-version '1.0.0.0' `
    --set-version-string FileDescription 'Novoline Launcher' `
    --set-version-string ProductName 'Novoline Launcher' `
    --set-version-string CompanyName 'Novoline' `
    --set-version-string OriginalFilename 'NovolineLauncher.exe' `
    --set-version-string InternalName 'NovolineLauncher' `
    --set-version-string LegalCopyright 'Copyright (C) 2026'
if ($LASTEXITCODE -ne 0) { throw "rcedit failed with exit code $LASTEXITCODE" }

$dist = Join-Path $root 'dist'
New-Item -ItemType Directory -Force -Path $dist | Out-Null
Copy-Item -LiteralPath $out -Destination (Join-Path $dist 'NovolineLauncher.exe') -Force

Write-Host "Built: $out"
Write-Host ("Size:  {0:N0} bytes" -f (Get-Item -LiteralPath $out).Length)
