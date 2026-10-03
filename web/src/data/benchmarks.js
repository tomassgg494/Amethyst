export const LANG_META = {
  amethyst: { label: 'Amethyst', color: '#a855f7' },
  c: { label: 'C (-O0)', color: '#67e8f9' },
  javascript: { label: 'JavaScript', color: '#fbbf24' },
  python: { label: 'Python', color: '#4ade80' },
}

export const LANG_ORDER = ['amethyst', 'c', 'javascript', 'python']

export const BENCHMARKS = [
  {
    id: 'fib',
    name: 'Recursive fibonacci',
    workload: 'fib(35)',
    times: { amethyst: 0.077, c: 0.0684, python: 0.7689, javascript: 0.1085 },
  },
  {
    id: 'loop',
    name: 'Hot integer loop',
    workload: '3×10⁸ iterações · % * +',
    times: { amethyst: 2.3921, c: 0.7936, python: 29.0681, javascript: 0.8541 },
  },
  {
    id: 'nested',
    name: 'Nested loops',
    workload: '4500 × 4500 iterações',
    times: { amethyst: 0.0868, c: 0.0637, python: 2.1322, javascript: 0.0617 },
  },
  {
    id: 'prime',
    name: 'Prime counting',
    workload: 'divisão trial ≤ 50 000',
    times: { amethyst: 0.0078, c: 0.0083, python: 0.0681, javascript: 0.0417 },
  },
]

export const BENCH_META = {
  date: '2026-09-22',
  machine: 'x86_64 · Linux',
  note: 'C com gcc -O0 (justo: o codegen da Amethyst não tem otimizador). Best of N.',
}

export function formatTime(sec) {
  if (sec < 0.001) return `${(sec * 1e6).toFixed(0)} µs`
  if (sec < 1) return `${(sec * 1000).toFixed(1)} ms`
  return `${sec.toFixed(2)} s`
}

/** Speed factor vs Amethyst: how many times faster is lang than Amethyst (or vice versa). */
export function speedup(vs, baseline) {
  // returns times[baseline] / times[vs] — >1 means vs is faster than baseline
  return vs / baseline
}
