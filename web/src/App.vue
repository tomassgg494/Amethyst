<script setup>
import AiAssistant from './components/AiAssistant.vue'
</script>

<template>
  <header class="site-header">
    <div class="container header-inner">
      <router-link to="/" class="brand">
        <svg class="brand__gem" viewBox="0 0 32 32" aria-hidden="true">
          <path d="M16 2 L28 10 L24 28 H8 L4 10 Z" fill="url(#bg)" />
          <path d="M16 2 L28 10 L16 14 L4 10 Z" fill="#e9d5ff" opacity="0.9" />
          <defs>
            <linearGradient id="bg" x1="4" y1="2" x2="28" y2="28" gradientUnits="userSpaceOnUse">
              <stop stop-color="#a855f7" />
              <stop offset="1" stop-color="#6d28d9" />
            </linearGradient>
          </defs>
        </svg>
        <span class="brand__name">Amethyst</span>
        <span class="brand__tag">v1</span>
      </router-link>

      <nav class="nav">
        <router-link to="/" class="nav__link" exact-active-class="is-active">
          Home
        </router-link>
        <router-link to="/docs" class="nav__link" active-class="is-active">
          Docs
        </router-link>
        <a
          class="nav__link nav__link--ext"
          href="https://github.com"
          target="_blank"
          rel="noreferrer"
        >
          GitHub
        </a>
      </nav>
    </div>
  </header>

  <main class="site-main">
    <router-view />
  </main>

  <footer class="site-footer">
    <div class="container footer-inner">
      <p>Amethyst — compilada, rápida, minimamente legível.</p>
      <p class="footer-meta">C++ · x86-64 · GAS · MIT</p>
    </div>
  </footer>

  <AiAssistant />
</template>

<style scoped>
.site-header {
  position: sticky;
  top: 0;
  z-index: 50;
  height: var(--header-h);
  backdrop-filter: blur(14px);
  background: rgba(12, 10, 18, 0.82);
  border-bottom: 1px solid var(--border);
}

.header-inner {
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.brand {
  display: inline-flex;
  align-items: center;
  gap: 0.55rem;
  color: var(--text);
  text-decoration: none !important;
  font-weight: 700;
}

.brand__gem {
  width: 28px;
  height: 28px;
}

.brand__name {
  font-size: 1.1rem;
  letter-spacing: -0.02em;
}

.brand__tag {
  font-size: 0.7rem;
  font-weight: 600;
  color: var(--amethyst-bright);
  border: 1px solid rgba(168, 85, 247, 0.45);
  background: rgba(168, 85, 247, 0.12);
  padding: 0.1rem 0.45rem;
  border-radius: 999px;
}

.nav {
  display: flex;
  align-items: center;
  gap: 0.35rem;
}

.nav__link {
  color: var(--text-dim);
  font-size: 0.92rem;
  font-weight: 500;
  padding: 0.45rem 0.85rem;
  border-radius: 8px;
  text-decoration: none !important;
  transition: color 0.15s, background 0.15s;
}

.nav__link:hover {
  color: var(--text);
  background: rgba(168, 85, 247, 0.1);
}

.nav__link.is-active {
  color: var(--amethyst-bright);
  background: rgba(168, 85, 247, 0.14);
}

.site-main {
  flex: 1;
}

.site-footer {
  border-top: 1px solid var(--border);
  padding: 2rem 0;
  margin-top: 4rem;
  background: var(--bg-soft);
}

.footer-inner {
  display: flex;
  flex-wrap: wrap;
  justify-content: space-between;
  gap: 0.75rem;
  align-items: center;
}

.footer-inner p {
  margin: 0;
  font-size: 0.9rem;
}

.footer-meta {
  color: var(--text-faint);
  font-family: var(--mono);
  font-size: 0.8rem !important;
}
</style>
