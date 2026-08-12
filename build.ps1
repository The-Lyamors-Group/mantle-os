param([ValidateSet('personal','education','government')][string]$Profile = 'personal')
$ErrorActionPreference = 'Stop'
if (-not (Get-Command docker -ErrorAction SilentlyContinue)) { throw "Docker Desktop est requis pour construire l’ISO depuis Windows." }
$root = (Resolve-Path $PSScriptRoot).Path
$escaped = $root.Replace("\", "/")
docker build -t mantleos-builder $root
$volume = $escaped + ":/workspace"
docker run --rm --privileged --env "MANTLE_PROFILE=$Profile" --volume $volume mantleos-builder
Write-Host ("ISO créée : " + (Join-Path $root "build/out/mantleos-amd64.iso"))
