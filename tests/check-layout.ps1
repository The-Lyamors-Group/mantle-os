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
Write-Output 'MantleOS layout checks: OK'
