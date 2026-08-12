<?php
declare(strict_types=1);
require_once __DIR__ . '/../xyz/secrets.def.php';
include_once __DIR__ . '/../xyz/info.dev.php';
?>
<!doctype html>
<html lang="fr">
<head>
  <meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="description" content="MantleOS — système d’exploitation libre, local et souverain.">
  <title>MantleOS — système local, lisible, souverain</title>
  <link rel="stylesheet" href="assets/site.css">
</head>
<body>
<div class="site-shell">
  <aside class="sidebar" aria-label="Navigation MantleOS">
    <a class="brand" href="#top"><span class="brand-mark">M</span><span><strong>MANTLEOS</strong><small>documentation locale · 0.1</small></span></a>
    <p class="nav-label">Explorer</p>
    <nav class="nav"><a class="active" href="#top">Accueil</a><a href="#principes">Principes</a><a href="#commandes">Commandes</a><a href="#scripts">Formats .mt</a><a href="#paquets">Paquets</a><a href="#editions">Éditions</a><a href="#build">Construire</a></nav>
    <div class="sidebar-foot">Aucune télémétrie.<br>Aucun CDN. Cette page fonctionne hors ligne.</div>
  </aside>
  <main class="main" id="top">
    <header class="topbar"><label class="search"><span>/</span><input data-search type="search" placeholder="Rechercher dans la documentation…" aria-label="Rechercher"></label><button class="theme-toggle" data-theme-toggle type="button">Mode sombre</button></header>
    <div class="content">
      <section class="hero" aria-labelledby="hero-title"><div><span class="eyebrow">Système d’exploitation européen · expérimental</span><h1 id="hero-title">Un système qui reste à vous.</h1><p class="lede">MantleOS construit une plateforme locale, lisible et durable : un noyau compilé depuis les sources, un userspace maîtrisé et des outils Unix qui restent familiers.</p><div class="meta"><span class="pill">x86_64 UEFI</span><span class="pill">Wayland en préparation</span><span class="pill">hors ligne par défaut</span></div></div><div class="hero-mark" aria-label="Logo MantleOS"><svg viewBox="0 0 180 160" aria-hidden="true"><path d="M20 125V28l70 56 70-56v97" fill="none" stroke="currentColor" stroke-width="8" stroke-linejoin="round"/><path d="M30 143l60-46 60 46M55 80v28M125 80v28" fill="none" stroke="currentColor" stroke-width="6"/></svg><small>local / open / controlled</small></div></section>
      <section class="section" id="principes" data-searchable><div class="section-head"><div><span class="eyebrow">01 · Orientation</span><h2>La lisibilité comme fonctionnalité.</h2></div><p>MantleOS préfère les composants explicites, vérifiables et remplaçables aux services silencieux et aux dépendances obligatoires.</p></div><div class="grid"><article class="card"><span class="tag">Confidentialité</span><h3>Rien ne part sans intention.</h3><p>Pas de télémétrie obligatoire, pas de compte cloud, pas de profilage publicitaire. Les diagnostics restent facultatifs et locaux.</p></article><article class="card"><span class="tag">Souveraineté</span><h3>Des formats ouverts.</h3><p>Sources verrouillées, build reproductible, paquets signés et architecture qui peut évoluer vers des dépôts privés ou hors ligne.</p></article><article class="card"><span class="tag">Compatibilité</span><h3>Les connaissances restent utiles.</h3><p>Shell POSIX, Git, SSH, curl, make et les outils Unix sont intégrés lorsqu’ils sont réellement construits et vérifiés.</p></article></div></section>
      <section class="section" id="commandes" data-searchable><div class="section-head"><div><span class="eyebrow">02 · Réflexes Unix</span><h2>Les commandes que vous connaissez.</h2></div><p>Les implémentations sont compilées depuis leurs sources dans le profil de compatibilité MantleOS.</p></div><div class="command-list"><div class="command"><code>sudo curl URL</code><span>élévation contrôlée puis transfert HTTPS</span><button class="copy" data-copy="sudo curl https://example.org/file">Copier</button></div><div class="command"><code>git clone URL</code><span>récupérer un projet depuis GitHub, GitLab, Codeberg ou un serveur Git</span><button class="copy" data-copy="git clone https://codeberg.org/example/project.git">Copier</button></div><div class="command"><code>ssh user@host</code><span>administration distante avec OpenSSH</span><button class="copy" data-copy="ssh user@host">Copier</button></div><div class="command"><code>make · cmake</code><span>construire un projet local avec les outils présents dans le profil choisi</span><button class="copy" data-copy="make">Copier</button></div><div class="command"><code>mantlectl status</code><span>consulter l’état des services et du socle MantleOS</span><button class="copy" data-copy="mantlectl status">Copier</button></div></div></section>
      <section class="section" id="scripts" data-searchable><div class="section-head"><div><span class="eyebrow">03 · Automation</span><h2>Deux formats, deux intentions.</h2></div><p><strong>.mt</strong> accepte une logique de script simple ; <strong>.mtc</strong> reste une séquence déclarative de commandes.</p></div><div class="grid"><article class="card"><span class="tag">install.mt</span><h3>Script MantleOS</h3><p>Le shebang officiel est <code>#!/usr/bin/mantle</code>. Une exécution téléchargée n’est jamais autorisée automatiquement.</p><div class="code-block" style="margin-top:16px"><div class="code-label">mantle script.mt</div><code>#!/usr/bin/mantle
print "Préparation"
sudo mantle install git
git clone URL
mantle build</code></div></article><article class="card"><span class="tag">update.mtc</span><h3>Séquence contrôlée</h3><p>Un format volontairement plus petit pour les tâches linéaires, sans introduire une seconde grammaire de shell.</p><div class="code-block" style="margin-top:16px"><div class="code-label">mantle exec update.mtc</div><code>mantle update
mantle upgrade
mantle clean</code></div></article><article class="card"><span class="tag">Sécurité</span><h3>Autorisation explicite.</h3><p><code>mantle run</code> et <code>mantle exec</code> rendent l’intention visible. Les dépôts clonés perdent les bits exécutables de leurs scripts .mt et .mtc.</p></article></div></section>
      <section class="section" id="paquets" data-searchable><div class="section-head"><div><span class="eyebrow">04 · Logiciels</span><h2>Des paquets vérifiables.</h2></div><p>Le format .mtpkg sépare les métadonnées, le manifeste et le payload. Une clé absente ou une signature invalide bloque l’installation.</p></div><div class="grid"><article class="card"><h3>Installer</h3><p><code>sudo mantle install package.mtpkg</code></p></article><article class="card"><h3>Vérifier</h3><p><code>mantle verify package.mtpkg</code><br>SHA-256 + signature OpenSSL.</p></article><article class="card"><h3>Source</h3><p><code>mantle source URL</code><br>Un dépôt est du code non fiable jusqu’à validation.</p></article></div></section>
      <section class="section" id="editions" data-searchable><div class="section-head"><div><span class="eyebrow">05 · Profils</span><h2>Un socle, trois contextes.</h2></div><p>Les profils partagent la même chaîne de boot et de userspace, puis appliquent des politiques adaptées.</p></div><div class="grid"><article class="card edition"><span class="tag">Personal</span><h3>Simple et privé.</h3><p>Pour un poste quotidien rapide, accessible et contrôlable sans compte en ligne obligatoire.</p></article><article class="card edition"><span class="tag">Education</span><h3>Administrable avec consentement.</h3><p>Prépare les politiques d’établissement, les comptes institutionnels et le déploiement centralisé sans surveillance clandestine.</p></article><article class="card edition"><span class="tag">Government</span><h3>Réduit et hors ligne.</h3><p>Dépôts internes, supports amovibles contrôlés, politiques renforcées et mises à jour hors ligne.</p></article></div></section>
      <section class="section" id="build" data-searchable><div class="section-head"><div><span class="eyebrow">06 · Développement</span><h2>Construire depuis les sources.</h2></div><p>La chaîne de build ne récupère pas un rootfs de distribution. Elle assemble musl, BusyBox, les composants MantleOS et le profil de compatibilité demandé.</p></div><div class="code-block"><div class="code-label">build MantleOS</div><code>docker build -t mantleos-builder .
docker run --rm --privileged \
  -e MANTLE_PROFILE=personal \
  -e MANTLE_COMPAT=full \
  -e MANTLE_SDK=0 \
  -v "$PWD:/workspace" mantleos-builder

# Vérifier l’image
sh ./tests/verify-image.sh
# Boot UEFI automatisé
make test-boot</code></div></section>
  <div data-udata-dataset="6a4ac3166a31224801b10c45"></div><script data-udata="https://www.data.gouv.fr" src="https://www.data.gouv.fr/oembed.js" async defer></script>

      <footer class="footer"><span>MantleOS 0.1 · prototype console et compatibilité en construction</span><span>Licence des composants : voir les sources et notices embarquées.</span></footer>
    </div>
  </main>
</div>
<script src="assets/site.js"></script>
</body>
</html>
