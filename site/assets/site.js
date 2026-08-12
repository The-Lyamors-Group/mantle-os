(function () {
  const root = document.documentElement;
  const toggle = document.querySelector('[data-theme-toggle]');
  const stored = localStorage.getItem('mantle-theme');
  if (stored) root.dataset.theme = stored;
  toggle?.addEventListener('click', () => {
    const next = root.dataset.theme === 'dark' ? 'light' : 'dark';
    root.dataset.theme = next;
    localStorage.setItem('mantle-theme', next);
    toggle.textContent = next === 'dark' ? 'Mode clair' : 'Mode sombre';
  });
  const search = document.querySelector('[data-search]');
  const searchable = [...document.querySelectorAll('[data-searchable]')];
  search?.addEventListener('input', () => {
    const term = search.value.trim().toLowerCase();
    searchable.forEach((item) => item.classList.toggle('hidden', term && !item.textContent.toLowerCase().includes(term)));
  });
  document.querySelectorAll('[data-copy]').forEach((button) => button.addEventListener('click', async () => {
    await navigator.clipboard.writeText(button.dataset.copy);
    const original = button.textContent; button.textContent = 'Copié';
    setTimeout(() => { button.textContent = original; }, 1100);
  }));
  const sections = [...document.querySelectorAll('section[id]')];
  const links = [...document.querySelectorAll('.nav a')];
  const observer = new IntersectionObserver((entries) => entries.forEach((entry) => {
    if (entry.isIntersecting) links.forEach((link) => link.classList.toggle('active', link.getAttribute('href') === '#' + entry.target.id));
  }), { rootMargin: '-25% 0px -65% 0px' });
  sections.forEach((section) => observer.observe(section));
}());
