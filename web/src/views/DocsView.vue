<script setup>
const links = [
  { to: '/docs', label: 'Introdução', exact: true },
  { to: '/docs/beginners', label: 'For Beginners' },
  { to: '/docs/install', label: 'Instalação' },
  { to: '/docs/syntax', label: 'Sintaxe' },
  { to: '/docs/types', label: 'Tipos' },
  { to: '/docs/statements', label: 'Statements' },
  { to: '/docs/expressions', label: 'Expressões' },
  { to: '/docs/compiler', label: 'Compilador' },
]
</script>

<template>
  <div class="docs container">
    <aside class="docs__sidebar">
      <p class="sidebar__title">Documentação</p>
      <nav class="sidebar__nav">
        <router-link
          v-for="link in links"
          :key="link.to"
          :to="link.to"
          class="sidebar__link"
          :class="{ 'is-exact': link.exact }"
          exact-active-class="is-active"
          active-class="is-active"
        >
          {{ link.label }}
        </router-link>
      </nav>
    </aside>

    <div class="docs__content prose">
      <router-view />
    </div>
  </div>
</template>

<style scoped>
.docs {
  display: grid;
  grid-template-columns: 220px 1fr;
  gap: 3rem;
  padding-top: 2.5rem;
  padding-bottom: 4rem;
  align-items: start;
}

.docs__sidebar {
  position: sticky;
  top: calc(var(--header-h) + 1.25rem);
}

.sidebar__title {
  font-family: var(--mono);
  font-size: 0.72rem;
  text-transform: uppercase;
  letter-spacing: 0.1em;
  color: var(--text-faint);
  margin: 0 0 0.85rem;
}

.sidebar__nav {
  display: flex;
  flex-direction: column;
  gap: 0.2rem;
  border-left: 1px solid var(--border);
}

.sidebar__link {
  color: var(--text-dim);
  font-size: 0.9rem;
  padding: 0.45rem 0.9rem;
  text-decoration: none !important;
  border-left: 2px solid transparent;
  margin-left: -1px;
  transition: color 0.15s, border-color 0.15s, background 0.15s;
}

.sidebar__link:hover {
  color: var(--text);
  background: rgba(168, 85, 247, 0.06);
}

.sidebar__link.is-active {
  color: var(--amethyst-bright);
  border-left-color: var(--amethyst);
  background: rgba(168, 85, 247, 0.1);
  font-weight: 600;
}

.docs__content {
  min-width: 0;
  max-width: 720px;
}

.docs__content :deep(h1) {
  margin: 0 0 1rem;
  font-size: 2rem;
}

.docs__content :deep(h1 + p) {
  font-size: 1.05rem;
  margin-top: 0;
}

@media (max-width: 800px) {
  .docs {
    grid-template-columns: 1fr;
    gap: 1.5rem;
  }

  .docs__sidebar {
    position: static;
  }

  .sidebar__nav {
    flex-direction: row;
    flex-wrap: wrap;
    border-left: none;
    gap: 0.4rem;
  }

  .sidebar__link {
    border: 1px solid var(--border);
    border-radius: 999px;
    margin-left: 0;
    padding: 0.35rem 0.8rem;
    font-size: 0.82rem;
  }

  .sidebar__link.is-active {
    border-color: var(--amethyst);
  }
}
</style>
