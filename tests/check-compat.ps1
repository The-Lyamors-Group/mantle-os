$ErrorActionPreference = 'Stop'
$required = @(
  'compat/build-compat.sh',
  'compat/build-sdk.sh',
  'compat/sources.lock',
  'services/mantle.c',
  'tests/fixtures/hello.mt',
  'tests/fixtures/update.mtc',
  'site/index.html',
  'site/assets/site.css',
  'site/assets/site.js',
  'ui/design-system/tokens.css',
  'site/assets/tokens.css',
  'apps/mime/mantle.xml',
  'apps/mime/mantle-script.desktop',
  'shell/mantle-shell.c',
  'scripting/mantle-script.c',
  'scripting/mantle-command.c',
  'docs/mantle-script.md',
  'docs/mantle-package.md',
  'tests/fixtures/security.mt'
)
foreach ($path in $required) { if (-not (Test-Path $path)) { throw "Fichier absent: $path" } }
$mt = Get-Content -Raw tests/fixtures/hello.mt
if ($mt -notmatch '^#!/usr/bin/mantle') { throw 'Shebang .mt invalide' }
if ((Get-Content -Raw site/index.html) -match '(src|href|@import)\s*=?.*https?://') { throw 'Dépendance distante dans le site local' }
Write-Output 'MantleOS compatibility checks: OK'
