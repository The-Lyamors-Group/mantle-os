$ErrorActionPreference = 'Stop'
$required = @(
  'kernel/config.x86_64',
  'init/mantle-init.c',
  'build/make-image.sh',
  'boot/grub/grub.cfg',
  'system/mantle-root-init.c',
  'system/mantle-network.c',
  'system/mantle-logd.c',
  'services/mantle-supervise.c',
  'services/mantlectl.c',
  'sources.lock'
)
foreach ($path in $required) { if (-not (Test-Path $path)) { throw "Fichier absent: $path" } }
if ((Select-String -Path build.sh -Pattern 'live-build|debootstrap|lb config' -Quiet)) { throw 'Ancienne chaîne live-build encore utilisée' }
if ((Select-String -Path build/make-image.sh -Pattern 'apt-get|debootstrap|live-build' -Quiet)) { throw "Le builder image dépend d’une distribution" }
$fetch = Get-Content build/make-image.sh -Raw
foreach ($flag in @('--fail-with-body','--location','--retry 5','--retry-all-errors','--retry-delay 2','--connect-timeout 20','--max-time 900','--proto ''=https''','--tlsv1.2')) {
  if ($fetch -notmatch [regex]::Escape($flag)) { throw "Option fetch absente: $flag" }
}
if ($fetch -match '(?i)curl\s+[^\n]*(--insecure|-k)') { throw 'Contournement TLS interdit détecté' }
$records = Get-Content sources.lock | Where-Object { $_ -and $_ -notmatch '^\s*#' }
foreach ($record in $records) {
  $fields = $record -split '\s+'
  if ($fields.Count -ne 4 -or $fields[2] -notmatch '^[0-9a-fA-F]{64}$' -or $fields[3] -notmatch '^https://') { throw "Entrée sources.lock invalide: $record" }
}
Write-Output 'MantleOS layout checks: OK'
