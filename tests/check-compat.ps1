$ErrorActionPreference = 'Stop'
foreach ($path in @('site/index.php', 'site/assets/site.css', 'site/assets/site.js')) {
  if (-not (Test-Path $path)) { throw "Fichier absent: $path" }
}
Write-Output 'MantleOS offline site checks: OK'
