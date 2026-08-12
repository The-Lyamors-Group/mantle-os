<?php
declare(strict_types=1);

$release = [
    'available' => false,
    'version' => '',
    'architecture' => 'x86_64 / amd64',
    'status' => 'Aucune version publique',
    'iso_url' => 'https://github.com/The-Lyamors-Group/mantle-os/releases/latest/download/mantleos-amd64.iso',
    'sha_url' => 'https://github.com/The-Lyamors-Group/mantle-os/releases/latest/download/mantleos-amd64.iso.sha256',
    'releases_url' => 'https://github.com/The-Lyamors-Group/mantle-os/releases',
    'dev_url' => 'https://github.com/The-Lyamors-Group/mantle-os/actions/workflows/mantleos-build.yml',
];

$isoUrl = htmlspecialchars($release['iso_url'], ENT_QUOTES, 'UTF-8');
$shaUrl = htmlspecialchars($release['sha_url'], ENT_QUOTES, 'UTF-8');
$releasesUrl = htmlspecialchars($release['releases_url'], ENT_QUOTES, 'UTF-8');
$devUrl = htmlspecialchars($release['dev_url'], ENT_QUOTES, 'UTF-8');
?>
<!doctype html>
<html lang="fr">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="description" content="MantleOS — système d’exploitation français open source, local et expérimental.">
  <title>MantleOS — un système qui reste à vous</title>
  <link rel="stylesheet" href="assets/site.css">
</head>
<body>
<div class="site-shell">
  <aside class="sidebar" aria-label="Navigation MantleOS">
    <a class="brand" href="#top">
      <span class="brand-mark" aria-hidden="true">M</span>
      <span><strong>MANTLEOS</strong><small>prototype indépendant · x86_64</small></span>
    </a>
    <p class="nav-label">Explorer</p>
    <nav class="nav">
      <a class="active" href="#top">Accueil</a>
      <a href="#download">Télécharger</a>
      <a href="#discover">Découvrir</a>
      <a href="#status">État du projet</a>
      <a href="#developers">Développer</a>
    </nav>
    <div class="sidebar-foot">
      <span class="privacy-mark">●</span> Pas de télémétrie obligatoire.<br>
      Documentation disponible hors ligne.
    </div>
  </aside>

  <main class="main" id="top">
    <header class="topbar">
      <label class="search"><span>/</span><input data-search type="search" placeholder="Rechercher dans la documentation…" aria-label="Rechercher dans la documentation"></label>
      <button class="theme-toggle" data-theme-toggle type="button">Mode sombre</button>
    </header>

    <div class="content">
      <section class="hero" aria-labelledby="hero-title">
        <div class="hero-copy">
          <div class="eyebrow-row"><span class="eyebrow">Système d’exploitation français · open source</span><span class="status-dot">Expérimental</span></div>
          <h1 id="hero-title">Un système qui reste à vous.</h1>
          <p class="lede">MantleOS construit une base indépendante, lisible et locale pour explorer une autre manière de faire fonctionner un ordinateur.</p>
          <div class="hero-actions">
            <?php if ($release['available']): ?>
              <a class="button button-primary" href="<?= $isoUrl ?>">Télécharger MantleOS <span aria-hidden="true">↓</span></a>
            <?php else: ?>
              <button class="button button-primary" type="button" disabled aria-disabled="true">Téléchargement bientôt disponible</button>
            <?php endif; ?>
            <a class="button button-secondary" href="#discover">Découvrir MantleOS</a>
          </div>
          <p class="hero-note"><strong>État actuel :</strong> kernel compilé et ISO générée dans la CI ; validation du démarrage UEFI encore en cours.</p>
        </div>
        <div class="hero-mark" aria-label="Logo MantleOS">
          <svg viewBox="0 0 180 160" aria-hidden="true"><path d="M20 125V28l70 56 70-56v97" fill="none" stroke="currentColor" stroke-width="8" stroke-linejoin="round"/><path d="M30 143l60-46 60 46M55 80v28M125 80v28" fill="none" stroke="currentColor" stroke-width="6"/></svg>
          <small>local / open / controlled</small>
        </div>
      </section>

      <section class="section download-section" id="download" data-searchable aria-labelledby="download-title">
        <div class="section-head">
          <div><span class="eyebrow">01 · Obtenir MantleOS</span><h2 id="download-title">Télécharger quand la version sera prête.</h2></div>
          <p>Une ISO est un fichier qui permet de démarrer un système dans une machine virtuelle ou depuis un support externe.</p>
        </div>
        <?php if ($release['available']): ?>
          <div class="download-card download-ready">
            <div class="download-main"><span class="download-badge ready">Version validée</span><h3>MantleOS <?= htmlspecialchars($release['version'], ENT_QUOTES, 'UTF-8') ?></h3><p>Image <?= htmlspecialchars($release['architecture'], ENT_QUOTES, 'UTF-8') ?> · publication GitHub Release</p></div>
            <div class="download-actions"><a class="button button-primary" href="<?= $isoUrl ?>">Télécharger l’ISO</a><a class="text-link" href="<?= $shaUrl ?>">SHA-256 officiel</a></div>
          </div>
        <?php else: ?>
          <div class="download-card download-pending">
            <div class="download-main"><span class="download-badge pending">Pas encore publié</span><h3>Aucune version publique n’est encore disponible.</h3><p>La compilation et la création de l’ISO sont en place. La publication sera faite uniquement après un démarrage UEFI réussi dans QEMU.</p></div>
            <div class="download-actions"><a class="button button-secondary" href="<?= $devUrl ?>">Voir les builds de développement</a><a class="text-link" href="<?= $releasesUrl ?>">Voir toutes les versions</a></div>
          </div>
        <?php endif; ?>
        <div class="download-meta">
          <div><span>Architecture cible</span><strong>x86_64 / amd64</strong></div>
          <div><span>Statut</span><strong>Build expérimental</strong></div>
          <div><span>SHA-256</span><strong>Publié avec chaque Release</strong></div>
        </div>
        <details class="verify-details"><summary>Comment vérifier une ISO téléchargée ?</summary><p>L’empreinte SHA-256 permet de vérifier que le fichier n’a pas été modifié pendant le téléchargement.</p><div class="verify-grid"><div><span>Windows PowerShell</span><code>Get-FileHash .\mantleos-amd64.iso -Algorithm SHA256</code></div><div><span>Linux</span><code>sha256sum mantleos-amd64.iso</code></div></div></details>
      </section>

      <section class="section" id="discover" data-searchable aria-labelledby="discover-title">
        <div class="section-head"><div><span class="eyebrow">02 · Découvrir</span><h2 id="discover-title">Une fondation que l’on peut comprendre.</h2></div><p>MantleOS commence petit : un kernel propre, une chaîne de build visible et des limites clairement affichées.</p></div>
        <div class="grid">
          <article class="card feature-card"><span class="feature-index">01</span><span class="tag">Confidentialité</span><h3>Local par défaut.</h3><p>Aucune télémétrie obligatoire, publicité, compte cloud imposé ou analytics intégré dans ce site.</p></article>
          <article class="card feature-card"><span class="feature-index">02</span><span class="tag">Souveraineté</span><h3>Des briques explicites.</h3><p>Le kernel MantleOS est compilé depuis ce dépôt. Linux, BusyBox et un rootfs de distribution ne sont pas utilisés comme fondation.</p></article>
          <article class="card feature-card"><span class="feature-index">03</span><span class="tag">Compatibilité</span><h3>Sans promesse artificielle.</h3><p>Le futur userspace visera des habitudes Unix familières, mais les commandes et applications ne sont pas encore dans l’image actuelle.</p></article>
        </div>
      </section>

      <section class="section" id="status" data-searchable aria-labelledby="status-title">
        <div class="section-head"><div><span class="eyebrow">03 · Transparence</span><h2 id="status-title">L’état réel, en un coup d’œil.</h2></div><p>Présent dans le dépôt ne veut pas dire fonctionnel dans l’ISO.</p></div>
        <div class="status-table" role="table" aria-label="État des composants MantleOS">
          <div class="status-row status-header" role="row"><span role="columnheader">Composant</span><span role="columnheader">État</span></div>
          <div class="status-row" role="row"><span role="cell">Kernel MantleOS x86_64</span><span class="state state-green" role="cell">Compilé et linké</span></div>
          <div class="status-row" role="row"><span role="cell">ISO UEFI / GRUB</span><span class="state state-green" role="cell">Générée par la CI</span></div>
          <div class="status-row" role="row"><span role="cell">Boot QEMU</span><span class="state state-amber" role="cell">Validation en cours</span></div>
          <div class="status-row" role="row"><span role="cell">Console VGA et série</span><span class="state state-amber" role="cell">Expérimentale</span></div>
          <div class="status-row" role="row"><span role="cell">Userspace, réseau, interface graphique</span><span class="state state-muted" role="cell">À implémenter</span></div>
        </div>
      </section>

      <section class="section" id="developers" data-searchable aria-labelledby="developers-title">
        <div class="section-head"><div><span class="eyebrow">04 · Pour contribuer</span><h2 id="developers-title">Construire depuis les sources.</h2></div><p>Les instructions techniques sont séparées des informations destinées aux débutants.</p></div>
        <div class="developer-grid">
          <div class="code-block"><div class="code-label">Construire et tester</div><code>MANTLE_PROFILE=personal sh ./build.sh
sh ./tests/verify-image.sh
sh ./tests/qemu-boot.sh</code></div>
          <div class="dev-card"><span class="tag">Architecture actuelle</span><p>UEFI → GRUB → Multiboot2 → kernel/arch/x86_64 → VGA + COM1</p><a class="text-link" href="https://github.com/The-Lyamors-Group/mantle-os/blob/main/docs/ARCHITECTURE.md">Lire l’architecture</a></div>
        </div>
      </section>

      <section class="section principles" data-searchable aria-labelledby="principles-title">
        <div class="section-head"><div><span class="eyebrow">05 · Principes</span><h2 id="principles-title">Simple à dire, difficile à mériter.</h2></div><p>La confidentialité et la souveraineté sont des objectifs d’architecture, pas des certifications.</p></div>
        <div class="principle-line"><span>Pas de télémétrie obligatoire</span><span>Pas de publicité</span><span>Pas de compte cloud imposé</span><span>Code open source</span></div>
      </section>

      <footer class="footer"><span>MantleOS · prototype indépendant x86_64</span><span><a href="https://github.com/The-Lyamors-Group/mantle-os">Code source</a> · Licence MIT</span></footer>
    </div>
  </main>
</div>
<script src="assets/site.js"></script>
</body>
</html>
