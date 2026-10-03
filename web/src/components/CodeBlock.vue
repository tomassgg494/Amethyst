<script setup>
import { computed } from 'vue'

const props = defineProps({
  code: { type: String, required: true },
  filename: { type: String, default: 'main.amt' },
})

const KEYWORDS = new Set([
  'fn', 'var', 'return', 'if', 'else', 'while', 'print',
  'break', 'continue', 'for', 'in',
  'int', 'bool', 'void', 'true', 'false',
])

function escapeHtml(s) {
  return s
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
}

function highlight(src) {
  const out = []
  let i = 0
  const n = src.length

  while (i < n) {
    const ch = src[i]

    // comments
    if (ch === '/' && src[i + 1] === '/') {
      let j = i
      while (j < n && src[j] !== '\n') j++
      out.push(`<span class="tok-comment">${escapeHtml(src.slice(i, j))}</span>`)
      i = j
      continue
    }

    // string literals
    if (ch === '"') {
      let j = i + 1
      while (j < n && src[j] !== '"') {
        if (src[j] === '\\') j++
        j++
      }
      j = Math.min(j + 1, n)
      out.push(`<span class="tok-str">${escapeHtml(src.slice(i, j))}</span>`)
      i = j
      continue
    }

    // numbers
    if (/[0-9]/.test(ch)) {
      let j = i
      while (j < n && /[0-9]/.test(src[j])) j++
      out.push(`<span class="tok-num">${src.slice(i, j)}</span>`)
      i = j
      continue
    }

    // identifiers / keywords / types
    if (/[A-Za-z_]/.test(ch)) {
      let j = i
      while (j < n && /[A-Za-z0-9_]/.test(src[j])) j++
      const word = src.slice(i, j)
      if (KEYWORDS.has(word)) {
        if (word === 'int' || word === 'bool' || word === 'void') {
          out.push(`<span class="tok-type">${word}</span>`)
        } else if (word === 'true' || word === 'false') {
          out.push(`<span class="tok-bool">${word}</span>`)
        } else {
          out.push(`<span class="tok-kw">${word}</span>`)
        }
      } else if (src[j] === '(') {
        out.push(`<span class="tok-fn">${word}</span>`)
      } else {
        out.push(`<span class="tok-ident">${word}</span>`)
      }
      i = j
      continue
    }

    // multi-char operators
    const two = src.slice(i, i + 2)
    if (['==', '!=', '<=', '>=', '&&', '||', '->', '..'].includes(two)) {
      out.push(`<span class="tok-op">${two}</span>`)
      i += 2
      continue
    }

    if ('+-*/%=<>!'.includes(ch)) {
      out.push(`<span class="tok-op">${ch}</span>`)
      i++
      continue
    }

    if ('(){}[],:;'.includes(ch)) {
      out.push(`<span class="tok-punct">${escapeHtml(ch)}</span>`)
      i++
      continue
    }

    out.push(escapeHtml(ch))
    i++
  }

  return out.join('')
}

const html = computed(() => highlight(props.code))
</script>

<template>
  <div class="code-block">
    <div class="code-block__bar">
      <span class="code-block__dot code-block__dot--r"></span>
      <span class="code-block__dot code-block__dot--y"></span>
      <span class="code-block__dot code-block__dot--g"></span>
      <span>{{ filename }}</span>
    </div>
    <!-- eslint-disable-next-line vue/no-v-html -->
    <pre><code v-html="html"></code></pre>
  </div>
</template>
