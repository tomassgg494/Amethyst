<script setup>
import { computed, ref } from 'vue'
import {
  BENCHMARKS,
  BENCH_META,
  LANG_META,
  LANG_ORDER,
  formatTime,
} from '../data/benchmarks.js'

const active = ref('fib')

const bench = computed(
  () => BENCHMARKS.find((b) => b.id === active.value) || BENCHMARKS[0],
)

const rows = computed(() => {
  const times = bench.value.times
  const max = Math.max(...Object.values(times))
  const best = Math.min(...Object.values(times))
  return LANG_ORDER.map((key) => {
    const t = times[key]
    return {
      key,
      label: LANG_META[key].label,
      color: LANG_META[key].color,
      time: t,
      // bar width relative to slowest
      width: (t / max) * 100,
      // ratio vs amethyst: "1.1×" etc. Show how Amethyst compares
      vsAmethyst:
        key === 'amethyst'
          ? null
          : t / times.amethyst, // >1 → Amethyst faster than this lang
      isFastest: t === best,
    }
  }).sort((a, b) => a.time - b.time)
})

const tabs = BENCHMARKS.map((b) => ({ id: b.id, name: b.name }))
</script>

<template>
  <section class="benchmarks" id="benchmarks">
    <div class="container">
      <h2 class="section-title">Benchmarks</h2>
      <p class="section-sub">
        4 testes básicos · a mesma lógica em 4 linguagens · best of 3
      </p>

      <div class="bench-tabs" role="tablist">
        <button
          v-for="tab in tabs"
          :key="tab.id"
          type="button"
          role="tab"
          class="bench-tab"
          :class="{ 'is-active': active === tab.id }"
          :aria-selected="active === tab.id"
          @click="active = tab.id"
        >
          {{ tab.name }}
        </button>
      </div>

      <div class="bench-card">
        <div class="bench-card__head">
          <div>
            <h3>{{ bench.name }}</h3>
            <p class="workload">
              <code>{{ bench.workload }}</code>
            </p>
          </div>
          <div class="bench-meta">
            <span>{{ BENCH_META.machine }}</span>
            <span>{{ BENCH_META.date }}</span>
          </div>
        </div>

        <ul class="bench-rows">
          <li v-for="row in rows" :key="row.key" class="bench-row">
            <div class="bench-row__label">
              <span class="swatch" :style="{ background: row.color }"></span>
              <span class="lang" :style="{ color: row.color }">{{ row.label }}</span>
              <span v-if="row.isFastest" class="badge-fast">mais rápido</span>
            </div>

            <div class="bench-row__bar-wrap">
              <div
                class="bench-row__bar"
                :style="{
                  width: row.width + '%',
                  background: `linear-gradient(90deg, ${row.color}cc, ${row.color})`,
                }"
              ></div>
            </div>

            <div class="bench-row__stats">
              <span class="time">{{ formatTime(row.time) }}</span>
              <span v-if="row.vsAmethyst" class="ratio">
                <template v-if="row.vsAmethyst > 1">
                  Amethyst <strong>{{ row.vsAmethyst.toFixed(1) }}×</strong> mais rápido
                </template>
                <template v-else-if="row.vsAmethyst < 1">
                  Amethyst <strong>{{ (1 / row.vsAmethyst).toFixed(1) }}×</strong> mais lento
                </template>
                <template v-else>empate</template>
              </span>
              <span v-else class="ratio ratio--self">referência desta coluna</span>
            </div>
          </li>
        </ul>

        <p class="bench-note">
          {{ BENCH_META.note }} Os outputs são verificados iguais entre as 4
          implementações antes de medir.
        </p>
      </div>

      <div class="bench-summary">
        <article v-for="b in BENCHMARKS" :key="b.id" class="sum-card">
          <h4>{{ b.name }}</h4>
          <p class="sum-work">{{ b.workload }}</p>
          <div class="sum-line">
            <span class="sum-vs">vs Python</span>
            <span class="sum-py">
              {{ (b.times.python / b.times.amethyst).toFixed(1) }}× mais rápido
            </span>
          </div>
          <div class="sum-line">
            <span class="sum-vs">vs C (-O0)</span>
            <span class="sum-c">
              <template v-if="b.times.amethyst <= b.times.c">
                {{ (b.times.c / b.times.amethyst).toFixed(1) }}× mais rápido
              </template>
              <template v-else>
                {{ (b.times.amethyst / b.times.c).toFixed(1) }}× mais lento
              </template>
            </span>
          </div>
        </article>
      </div>
    </div>
  </section>
</template>

<style scoped>
.benchmarks {
  padding: 4.5rem 0;
  background: var(--bg-soft);
  border-block: 1px solid var(--border);
}

.bench-tabs {
  display: flex;
  flex-wrap: wrap;
  gap: 0.5rem;
  justify-content: center;
  margin-bottom: 1.5rem;
}

.bench-tab {
  font-family: var(--sans);
  font-size: 0.88rem;
  font-weight: 600;
  color: var(--text-dim);
  background: var(--bg-card);
  border: 1px solid var(--border);
  border-radius: 999px;
  padding: 0.5rem 1.1rem;
  cursor: pointer;
  transition: all 0.15s;
}

.bench-tab:hover {
  border-color: var(--border-strong);
  color: var(--text);
}

.bench-tab.is-active {
  color: #fff;
  background: linear-gradient(135deg, var(--amethyst), var(--amethyst-deep));
  border-color: transparent;
  box-shadow: 0 4px 16px rgba(168, 85, 247, 0.35);
}

.bench-card {
  background: var(--bg-card);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  padding: 1.5rem 1.5rem 1.25rem;
  box-shadow: 0 12px 40px rgba(0, 0, 0, 0.25);
}

.bench-card__head {
  display: flex;
  flex-wrap: wrap;
  justify-content: space-between;
  gap: 1rem;
  margin-bottom: 1.35rem;
}

.bench-card__head h3 {
  margin: 0;
  font-size: 1.2rem;
}

.workload {
  margin: 0.3rem 0 0;
  font-size: 0.88rem;
}

.workload code {
  color: var(--amethyst-bright);
  background: rgba(168, 85, 247, 0.1);
  border: 1px solid rgba(168, 85, 247, 0.25);
  padding: 0.1rem 0.45rem;
  border-radius: 6px;
  font-size: 0.85em;
}

.bench-meta {
  display: flex;
  flex-direction: column;
  align-items: flex-end;
  gap: 0.2rem;
  font-family: var(--mono);
  font-size: 0.72rem;
  color: var(--text-faint);
}

.bench-rows {
  list-style: none;
  margin: 0;
  padding: 0;
  display: flex;
  flex-direction: column;
  gap: 1.1rem;
}

.bench-row {
  display: grid;
  grid-template-columns: 160px 1fr 220px;
  gap: 1rem;
  align-items: center;
}

.bench-row__label {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  min-width: 0;
}

.swatch {
  width: 10px;
  height: 10px;
  border-radius: 3px;
  flex-shrink: 0;
}

.lang {
  font-weight: 700;
  font-size: 0.9rem;
}

.badge-fast {
  font-size: 0.65rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.04em;
  color: var(--green);
  background: rgba(74, 222, 128, 0.12);
  border: 1px solid rgba(74, 222, 128, 0.35);
  padding: 0.1rem 0.4rem;
  border-radius: 999px;
  white-space: nowrap;
}

.bench-row__bar-wrap {
  height: 22px;
  background: rgba(255, 255, 255, 0.04);
  border-radius: 999px;
  overflow: hidden;
  border: 1px solid var(--border);
}

.bench-row__bar {
  height: 100%;
  border-radius: 999px;
  min-width: 4px;
  transition: width 0.45s ease;
}

.bench-row__stats {
  display: flex;
  flex-direction: column;
  align-items: flex-end;
  gap: 0.1rem;
}

.time {
  font-family: var(--mono);
  font-weight: 700;
  font-size: 0.95rem;
  color: var(--text);
}

.ratio {
  font-size: 0.75rem;
  color: var(--text-faint);
  text-align: right;
}

.ratio strong {
  color: var(--amethyst-bright);
}

.ratio--self {
  color: var(--text-faint);
}

.bench-note {
  margin: 1.35rem 0 0;
  font-size: 0.8rem;
  color: var(--text-faint);
  border-top: 1px dashed var(--border);
  padding-top: 1rem;
}

.bench-summary {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 0.85rem;
  margin-top: 1.25rem;
}

.sum-card {
  background: var(--bg-card);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  padding: 1rem 1.05rem;
}

.sum-card h4 {
  margin: 0;
  font-size: 0.88rem;
  color: var(--text);
}

.sum-work {
  margin: 0.2rem 0 0.75rem;
  font-size: 0.72rem;
  font-family: var(--mono);
  color: var(--text-faint);
}

.sum-line {
  display: flex;
  justify-content: space-between;
  align-items: baseline;
  gap: 0.35rem;
  font-size: 0.78rem;
  margin-top: 0.3rem;
}

.sum-line .sum-vs {
  color: var(--text-faint);
}

.sum-py {
  color: var(--green);
  font-weight: 600;
}

.sum-c {
  color: var(--cyan);
  font-weight: 600;
}

@media (max-width: 900px) {
  .bench-row {
    grid-template-columns: 1fr;
    gap: 0.4rem;
  }

  .bench-row__stats {
    align-items: flex-start;
  }

  .ratio {
    text-align: left;
  }

  .bench-summary {
    grid-template-columns: 1fr 1fr;
  }

  .bench-meta {
    align-items: flex-start;
  }
}

@media (max-width: 520px) {
  .bench-summary {
    grid-template-columns: 1fr;
  }
}
</style>
