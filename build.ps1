param(
    [ValidateSet("personal", "education", "government")]
    [string]$Profile = "personal",

    [ValidateSet("minimal", "full")]
    [string]$Compat = "full",

    [switch]$Sdk,
    [switch]$InstallTools,
    [switch]$BuildOnly,
    [switch]$TestOnly,
    [switch]$Interactive,
    [switch]$Clean,
    [switch]$NoQemu,
    [int]$MemoryMB = 512
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
# Windows PowerShell ne crée pas LASTEXITCODE tant qu'aucun processus natif
# n'a été lancé. L'initialiser évite qu'un contrôle PowerShell statique soit
# considéré comme une erreur de toolchain sous StrictMode.
$global:LASTEXITCODE = 0

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ProjectRoot

$BuildDir = Join-Path $ProjectRoot "build"
$OutDir   = Join-Path $BuildDir "out"
$WorkDir  = Join-Path $BuildDir "work"

$IsoPath       = Join-Path $OutDir "mantleos-amd64.iso"
$SerialLog     = Join-Path $OutDir "qemu-serial.log"
$QemuLog       = Join-Path $OutDir "qemu.log"
$BuildLog      = Join-Path $OutDir "build.log"
$BuildInfo     = Join-Path $OutDir "build-info.txt"

function Write-Step {
    param([string]$Message)
    Write-Host ""
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Write-Ok {
    param([string]$Message)
    Write-Host "[OK] $Message" -ForegroundColor Green
}

function Write-Warn {
    param([string]$Message)
    Write-Host "[WARN] $Message" -ForegroundColor Yellow
}

function Fail {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
    exit 1
}

function Test-Command {
    param([string]$Name)

    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function ConvertTo-MsysShellLiteral {
    param([Parameter(Mandatory = $true)][string]$Value)

    return "'" + ($Value -replace "'", "'\\''") + "'"
}

function Ensure-Directory {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        New-Item -ItemType Directory -Path $Path | Out-Null
    }
}

function Get-Msys2Bash {
    $Candidates = @(
        "C:\msys64\usr\bin\bash.exe",
        "C:\tools\msys64\usr\bin\bash.exe"
    )

    foreach ($Candidate in $Candidates) {
        if (Test-Path $Candidate) {
            return $Candidate
        }
    }

    return $null
}

function Invoke-MsysChecked {
    param(
        [Parameter(Mandatory = $true)][string]$Bash,
        [Parameter(Mandatory = $true)][string]$Command,
        [Parameter(Mandatory = $true)][string]$FailureMessage
    )

    & $Bash -lc $Command
    if ($LASTEXITCODE -ne 0) {
        Fail "$FailureMessage (code $LASTEXITCODE)"
    }
}

function Test-MsysToolchain {
    param(
        [Parameter(Mandatory = $true)][string]$Bash,
        [switch]$RequireQemu
    )

    Write-Step "Vérification de la toolchain MSYS2"
    $CheckCommand = 'for tool in make cc gcc ld as ar objcopy objdump readelf nm xorriso; do command -v "$tool" >/dev/null 2>&1 || printf "%s\n" "$tool"; done; command -v python3 >/dev/null 2>&1 || command -v python >/dev/null 2>&1 || printf "%s\n" python'
    $Missing = @(& $Bash -lc "export PATH=/mingw64/bin:`$PATH; $CheckCommand" | Where-Object { $_ -and $_.Trim() })
    if ($LASTEXITCODE -ne 0) {
        Fail "Impossible d'interroger la toolchain MSYS2 (code $LASTEXITCODE)"
    }

    if ($Missing.Count -gt 0) {
        Write-Host "Outils MSYS2 absents : $($Missing -join ', ')" -ForegroundColor Yellow
        Write-Host "Commande MSYS2 : pacman -S --needed --noconfirm base-devel git make cmake ninja gcc binutils nasm python curl wget xorriso mingw-w64-x86_64-clang mingw-w64-x86_64-lld" -ForegroundColor Yellow
        Fail "Toolchain MSYS2 incomplète : aucune compilation ne sera lancée"
    }

    $ElfToolchainCheck = @'
if command -v x86_64-elf-gcc >/dev/null 2>&1 && command -v x86_64-elf-ld >/dev/null 2>&1; then
    printf '%s\n' x86_64-elf-gcc
elif command -v clang >/dev/null 2>&1 && command -v ld.lld >/dev/null 2>&1 && printf '%s\n' "$(clang --target=x86_64-elf -print-target-triple 2>/dev/null)" | grep -Eq '^x86_64-.*-elf$'; then
    printf '%s\n' clang-elf
elif command -v gcc >/dev/null 2>&1 && command -v ld >/dev/null 2>&1 && [ "$(gcc -dumpmachine 2>/dev/null)" != x86_64-pc-cygwin ]; then
    printf '%s\n' gcc-elf
else
    exit 1
fi
'@
    $ElfToolchain = & $Bash -lc "export PATH=/mingw64/bin:`$PATH; $ElfToolchainCheck"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "GCC Cygwin PE/COFF est présent, mais aucun compilateur ELF x86_64 n'est disponible." -ForegroundColor Yellow
        Write-Host "Commande MSYS2 : pacman -S --needed --noconfirm mingw-w64-x86_64-clang mingw-w64-x86_64-lld" -ForegroundColor Yellow
        Write-Host "Puis ajoute /mingw64/bin au PATH du shell MSYS2; le build utilisera clang --target=x86_64-elf et ld.lld." -ForegroundColor Yellow
        Fail "Compilateur ELF x86_64 MantleOS absent"
    }
    Write-Host "ELF toolchain: $($ElfToolchain -join ', ')" -ForegroundColor DarkGray

    if ($RequireQemu -and -not (Resolve-Qemu)) {
        Write-Host "Commande MSYS2 possible pour QEMU : pacman -S --needed --noconfirm mingw-w64-x86_64-qemu" -ForegroundColor Yellow
        Fail "qemu-system-x86_64 est requis pour le test local"
    }

    Write-Ok "Toolchain MSYS2 détectée"
}

function Install-RequiredTools {
    Write-Step "Installation des outils de développement"

    if (-not (Test-Command "winget")) {
        Fail "winget est absent. Installe App Installer depuis Microsoft Store."
    }

    $MsysBash = Get-Msys2Bash

    if (-not $MsysBash) {
        Write-Host "Installation de MSYS2..."
        & winget install --id MSYS2.MSYS2 --exact --accept-source-agreements --accept-package-agreements
        if ($LASTEXITCODE -ne 0) {
            Fail "Installation de MSYS2 échouée (code $LASTEXITCODE)"
        }

        $MsysBash = Get-Msys2Bash

        if (-not $MsysBash) {
            Fail "MSYS2 a été installé mais bash.exe est introuvable. Relance PowerShell."
        }
    }

    Write-Host "Mise à jour MSYS2 et installation de la toolchain..."

    Invoke-MsysChecked -Bash $MsysBash -Command "pacman -Syu --noconfirm" -FailureMessage "Mise à jour MSYS2 échouée"

    $Packages = @"
pacman -S --needed --noconfirm \
  base-devel \
  git \
  make \
  cmake \
  ninja \
  mingw-w64-x86_64-clang \
  mingw-w64-x86_64-lld \
  mingw-w64-x86_64-llvm \
  gcc \
  binutils \
  nasm \
  python \
  curl \
  wget \
  xorriso
"@

    Invoke-MsysChecked -Bash $MsysBash -Command $Packages -FailureMessage "Installation de la toolchain MSYS2 échouée"
    Test-MsysToolchain -Bash $MsysBash

    if (-not (Resolve-Qemu)) {
        Write-Warn "QEMU n'est pas trouvé dans le PATH Windows."

        Write-Host "Tentative d'installation de QEMU via winget..."

        & winget install --id SoftwareFreedomConservancy.QEMU --exact --accept-source-agreements --accept-package-agreements
        if ($LASTEXITCODE -ne 0) {
            Fail "Installation automatique de QEMU échouée (code $LASTEXITCODE)"
        }
    }

    if (-not (Resolve-Qemu)) {
        Fail "QEMU reste introuvable après l'installation"
    }

    Write-Ok "Installation des outils terminée"
}

function Resolve-IsoBackend {
    param([Parameter(Mandatory = $true)][string]$Bash)

    & $Bash -lc 'command -v grub-mkrescue >/dev/null 2>&1 && command -v grub-file >/dev/null 2>&1'
    if ($LASTEXITCODE -eq 0) {
        return "grub"
    }

    $NativeLoader = Join-Path $ProjectRoot "boot\efi\loader.c"
    & $Bash -lc 'export PATH=/mingw64/bin:$PATH; command -v xorriso >/dev/null 2>&1 && command -v clang >/dev/null 2>&1 && command -v lld-link >/dev/null 2>&1'
    if ($LASTEXITCODE -eq 0 -and (Test-Path $NativeLoader -PathType Leaf)) {
        return "windows-native"
    }

    return $null
}

function Resolve-Qemu {
    $Command = Get-Command "qemu-system-x86_64" -ErrorAction SilentlyContinue

    if ($Command) {
        return $Command.Source
    }

    $Candidates = @(
        "C:\Program Files\qemu\qemu-system-x86_64.exe",
        "C:\Program Files\QEMU\qemu-system-x86_64.exe",
        "C:\msys64\ucrt64\bin\qemu-system-x86_64.exe",
        "C:\msys64\mingw64\bin\qemu-system-x86_64.exe"
    )

    foreach ($Candidate in $Candidates) {
        if (Test-Path $Candidate) {
            return $Candidate
        }
    }

    return $null
}

function Resolve-OVMF {
    $CodeCandidates = @(
        (Join-Path $ProjectRoot "firmware\OVMF_CODE_4M.fd"),
        "C:\Program Files\qemu\share\edk2-x86_64-code.fd",
        "C:\Program Files\qemu\share\edk2-x86_64-secure-code.fd",
        "C:\msys64\ucrt64\share\edk2\x64\OVMF_CODE.fd",
        "C:\msys64\mingw64\share\edk2\x64\OVMF_CODE.fd"
    )

    $VarsCandidates = @(
        (Join-Path $ProjectRoot "firmware\OVMF_VARS_4M.fd"),
        "C:\Program Files\qemu\share\edk2-i386-vars.fd",
        "C:\msys64\ucrt64\share\edk2\x64\OVMF_VARS.fd",
        "C:\msys64\mingw64\share\edk2\x64\OVMF_VARS.fd"
    )

    $Code = $null
    $Vars = $null

    foreach ($Candidate in $CodeCandidates) {
        if (Test-Path $Candidate) {
            $Code = $Candidate
            break
        }
    }

    foreach ($Candidate in $VarsCandidates) {
        if (Test-Path $Candidate) {
            $Vars = $Candidate
            break
        }
    }

    if (-not $Code -or -not $Vars) {
        return $null
    }

    return @{
        Code = $Code
        Vars = $Vars
    }
}

function Invoke-Msys {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Command
    )

    $Bash = Get-Msys2Bash

    if (-not $Bash) {
        Fail "MSYS2 introuvable. Lance build.ps1 -InstallTools."
    }

    $UnixProject = $ProjectRoot.Replace("\", "/")
    $Drive = $UnixProject.Substring(0, 1).ToLower()
    $UnixProject = "/$Drive" + $UnixProject.Substring(2)

    $FullCommand = "export PATH=/mingw64/bin:`$PATH; cd -- $(ConvertTo-MsysShellLiteral $UnixProject) && $Command"

    & $Bash -lc $FullCommand

    if ($LASTEXITCODE -ne 0) {
        throw "Commande MSYS2 échouée avec code $LASTEXITCODE"
    }
}

function Clean-Build {
    Write-Step "Nettoyage"

    if (Test-Path $WorkDir) {
        Remove-Item $WorkDir -Recurse -Force
    }

    if (Test-Path $OutDir) {
        Remove-Item $OutDir -Recurse -Force
    }

    Ensure-Directory $WorkDir
    Ensure-Directory $OutDir

    Write-Ok "Build nettoyé"
}

function Test-ProjectLayout {
    Write-Step "Vérification du dépôt"

    $Required = @(
        "kernel",
        "boot",
        "tests",
        "build.sh"
    )

    foreach ($Item in $Required) {
        $Path = Join-Path $ProjectRoot $Item

        if (-not (Test-Path $Path)) {
            Fail "Élément requis absent : $Item"
        }
    }

    Write-Ok "Structure du dépôt valide"
}

function Invoke-StaticChecks {
    Write-Step "Contrôles statiques"

    if (Test-Path ".\tests\check-layout.ps1") {
        & ".\tests\check-layout.ps1"

        if ($LASTEXITCODE -ne 0) {
            Fail "check-layout.ps1 a échoué"
        }
    }

    if (Test-Path ".\tests\check-compat.ps1") {
        & ".\tests\check-compat.ps1"

        if ($LASTEXITCODE -ne 0) {
            Fail "check-compat.ps1 a échoué"
        }
    }

    if (Test-Command "git") {
        git diff --check

        if ($LASTEXITCODE -ne 0) {
            Fail "git diff --check a détecté une erreur"
        }
    }

    Write-Ok "Contrôles statiques validés"
}

function Invoke-Build {
    param([Parameter(Mandatory = $true)][string]$IsoBackend)

    Write-Step "Compilation de MantleOS"

    Ensure-Directory $OutDir

    if (Test-Path $BuildLog) {
        Remove-Item $BuildLog -Force
    }

    $SdkFlag = if ($Sdk) { "1" } else { "0" }

    $Command = @"
export MANTLE_PROFILE='$Profile'
export MANTLE_COMPAT='$Compat'
export MANTLE_SDK='$SdkFlag'
export MANTLE_ISO_BACKEND='$IsoBackend'
set -o pipefail
sh ./build.sh 2>&1 | tee build/out/build.log
"@

    try {
        Invoke-Msys $Command
    }
    catch {
        Write-Host ""
        Write-Host "Dernières lignes du build :" -ForegroundColor Yellow

        if (Test-Path $BuildLog) {
            Get-Content $BuildLog -Tail 80
        }

        Fail "Compilation MantleOS échouée"
    }

    if (-not (Test-Path $IsoPath)) {
        Fail "Le build s'est terminé mais l'ISO est absente : $IsoPath"
    }

    Write-Ok "ISO générée : $IsoPath"
}

function Get-Sha256 {
    Write-Step "Calcul SHA-256"

    if (-not (Test-Path $IsoPath)) {
        Fail "ISO absente"
    }

    $Hash = (Get-FileHash $IsoPath -Algorithm SHA256).Hash.ToLowerInvariant()

    $HashFile = "$IsoPath.sha256"

    "$Hash  mantleos-amd64.iso" | Set-Content $HashFile -Encoding ASCII

    Write-Host "SHA-256: $Hash"
    Write-Ok "Empreinte enregistrée"
}

function Invoke-ImageVerification {
    Write-Step "Validation de l'image"

    if (-not (Test-Path $IsoPath)) {
        Fail "ISO absente"
    }

    if (Test-Path ".\tests\verify-image.sh") {
        Invoke-Msys "sh ./tests/verify-image.sh"
    }

    Write-Ok "Validation image terminée"
}

function Start-MantleQemu {
    param(
        [switch]$InteractiveMode
    )

    Write-Step "Démarrage MantleOS dans QEMU"

    $Qemu = Resolve-Qemu

    if (-not $Qemu) {
        Fail "QEMU introuvable. Installe-le puis relance."
    }

    $Firmware = Resolve-OVMF

    if (-not $Firmware) {
        Fail "Firmware OVMF/EDK2 introuvable."
    }

    if (-not (Test-Path $IsoPath)) {
        Fail "ISO MantleOS absente."
    }

    Ensure-Directory $OutDir

    "" | Set-Content $SerialLog -Encoding ASCII
    "" | Set-Content $QemuLog -Encoding ASCII

    $VarsCopy = Join-Path $OutDir "OVMF_VARS.fd"
    $CodeCopy = Join-Path $OutDir "OVMF_CODE.fd"
    Copy-Item $Firmware.Vars $VarsCopy -Force
    Copy-Item $Firmware.Code $CodeCopy -Force

    Write-Host "QEMU       : $Qemu"
    Write-Host "OVMF CODE  : $($Firmware.Code)"
    Write-Host "OVMF VARS  : $VarsCopy"
    Write-Host "ISO        : $IsoPath"

    $Args = @(
        "-machine", "q35",
        "-m", "$MemoryMB",
        "-drive", "if=pflash,format=raw,unit=0,readonly=on,file=OVMF_CODE.fd",
        "-drive", "if=pflash,format=raw,unit=1,file=OVMF_VARS.fd",
        "-cdrom", "mantleos-amd64.iso",
        "-serial", "file:qemu-serial.log",
        "-no-reboot"
    )

    if ($InteractiveMode) {
        $Args += @(
            "-display", "sdl"
        )
    }
    else {
        $Args += @(
            "-display", "none",
            "-monitor", "none"
        )
    }

    Write-Host ""
    Write-Host "$Qemu $($Args -join ' ')" -ForegroundColor DarkGray

    if ($InteractiveMode) {
        Push-Location $OutDir
        try { & $Qemu @Args } finally { Pop-Location }
        return
    }

    $Process = Start-Process `
        -FilePath $Qemu `
        -ArgumentList $Args `
        -NoNewWindow `
        -PassThru `
        -RedirectStandardError $QemuLog `
        -WorkingDirectory $OutDir

    $Deadline = (Get-Date).AddSeconds(45)

    $SuccessMarkers = @(
        "MANTLE_KERNEL_OK",
        "MANTLE_MB2_MAGIC_OK",
        "MANTLE_GRAPHICS_OK",
        "MANTLE_MB2_MODULE_FOUND",
        "MANTLE_ROOTFS_OK",
        "MANTLE_ELF_OK",
        "MANTLE_USERSPACE_OK",
        "MANTLE_INIT_USER_OK",
        "MANTLE_SHELL_CONSOLE_OK"
    )

    $FatalMarkers = @(
        "MANTLE_ROOTFS_ERROR",
        "MANTLE_ROOTFS_NOT_FOUND",
        "MANTLE_ROOTFS_INVALID",
        "MANTLE_ROOTFS_MAPPING_ERROR",
        "MANTLE_MB2_MAGIC_ERROR"
    )

    while ((Get-Date) -lt $Deadline) {
        Start-Sleep -Milliseconds 300

        if (Test-Path $SerialLog) {
            $Text = Get-Content $SerialLog -Raw -ErrorAction SilentlyContinue

            foreach ($Fatal in $FatalMarkers) {
                if ($Text -match [regex]::Escape($Fatal)) {
                    if (-not $Process.HasExited) {
                        Stop-Process $Process.Id -Force
                    }

                    Get-Content $SerialLog
                    Fail "MantleOS a signalé une erreur fatale : $Fatal"
                }
            }

            $AllSuccess = $true

            foreach ($Marker in $SuccessMarkers) {
                if ($Text -notmatch [regex]::Escape($Marker)) {
                    $AllSuccess = $false
                    break
                }
            }

            if ($AllSuccess) {
                if (-not $Process.HasExited) {
                    Stop-Process $Process.Id -Force
                }

                Write-Ok "Tous les marqueurs MantleOS ont été validés"
                Get-Content $SerialLog
                return
            }
        }

        if ($Process.HasExited) {
            break
        }
    }

    if (-not $Process.HasExited) {
        Stop-Process $Process.Id -Force
    }

    Write-Host ""
    Write-Host "===== qemu-serial.log =====" -ForegroundColor Yellow

    if (Test-Path $SerialLog) {
        Get-Content $SerialLog
    }

    Write-Host ""
    Write-Host "===== qemu.log =====" -ForegroundColor Yellow

    if (Test-Path $QemuLog) {
        Get-Content $QemuLog
    }

    Fail "Le boot QEMU n'a pas atteint tous les marqueurs attendus"
}

function Write-BuildInformation {
    Write-Step "Génération build-info.txt"

    $Commit = "unknown"

    if (Test-Command "git") {
        try {
            $Commit = (git rev-parse HEAD).Trim()
        }
        catch {}
    }

    $IsoHash = ""

    if (Test-Path $IsoPath) {
        $IsoHash = (Get-FileHash $IsoPath -Algorithm SHA256).Hash.ToLowerInvariant()
    }

    @"
MantleOS Build Information
==========================

Date: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
Profile: $Profile
Compatibility: $Compat
SDK: $($Sdk.IsPresent)
Architecture: x86_64
Git commit: $Commit
ISO: $IsoPath
ISO SHA-256: $IsoHash

Host:
$([Environment]::OSVersion.VersionString)

PowerShell:
$($PSVersionTable.PSVersion)
"@ | Set-Content $BuildInfo -Encoding UTF8

    Write-Ok "build-info.txt généré"
}

Write-Host ""
Write-Host "MantleOS Local Build System" -ForegroundColor Magenta
Write-Host "===========================" -ForegroundColor Magenta

if ($InstallTools) {
    Install-RequiredTools
}

Test-ProjectLayout

Ensure-Directory $BuildDir
Ensure-Directory $WorkDir
Ensure-Directory $OutDir

if ($Clean) {
    Clean-Build
}

Invoke-StaticChecks

if (-not $TestOnly) {
    $MsysBash = Get-Msys2Bash
    if (-not $MsysBash) {
        Fail "MSYS2 introuvable. Lance build.ps1 -InstallTools."
    }

    Test-MsysToolchain -Bash $MsysBash -RequireQemu:(-not $BuildOnly -and -not $NoQemu)
    $IsoBackend = Resolve-IsoBackend -Bash $MsysBash
    if (-not $IsoBackend) {
        Fail "Aucun backend ISO amorçable disponible : GRUB absent et boot/efi/loader.c absent. xorriso seul ne suffit pas."
    }
    Write-Host "ISO backend: $IsoBackend" -ForegroundColor Cyan
    Invoke-Build -IsoBackend $IsoBackend
    Invoke-ImageVerification
    Get-Sha256
    Write-BuildInformation
}

if (-not $TestOnly -and -not $BuildOnly -and -not $NoQemu) {
    Start-MantleQemu -InteractiveMode:$Interactive
}

Write-Host ""
Write-Ok "Pipeline MantleOS terminé"
