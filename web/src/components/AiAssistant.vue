<script setup>
import { nextTick, ref } from 'vue'

const API_KEY = 'AIzaSyDiRGDKlEUgFs95JH7EgaFyreRdsgUCdvk'
const MODEL = 'gemini-3-flash-preview'
const API_URL = `https://generativelanguage.googleapis.com/v1beta/models/${MODEL}:generateContent`

const SYSTEM_PROMPT = `Você é o assistente de IA oficial do site do Amethyst, uma linguagem de programação compilada (C++17, x86-64 GAS, sem LLVM) com tipos estáticos (int=64-bit, bool, void), sintaxe estilo C/JS/Python (fn, var, chaves, ponto-e-vírgula) e pipeline .amt → Lexer → Parser → Sema → Codegen → as/ld. Responda de forma curta, simpática e em português de Portugal. Ajuda com dúvidas sobre a linguagem, instalação, sintaxe e exemplos de código Amethyst.`

const open = ref(false)
const loading = ref(false)
const input = ref('')
const messagesEl = ref(null)
const messages = ref([
  {
    role: 'assistant',
    text: 'Olá! Sou o assistente do Amethyst. Pergunta-me sobre a linguagem, instalação, sintaxe ou pede um exemplo de código.',
  },
])

function toggle() {
  open.value = !open.value
}

async function scrollToBottom() {
  await nextTick()
  if (messagesEl.value) {
    messagesEl.value.scrollTop = messagesEl.value.scrollHeight
  }
}

async function send() {
  const text = input.value.trim()
  if (!text || loading.value) return

  messages.value.push({ role: 'user', text })
  input.value = ''
  loading.value = true
  await scrollToBottom()

  try {
    const contents = messages.value.map((m) => ({
      role: m.role === 'assistant' ? 'model' : 'user',
      parts: [{ text: m.text }],
    }))

    const res = await fetch(API_URL, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'x-goog-api-key': API_KEY,
      },
      body: JSON.stringify({
        systemInstruction: { parts: [{ text: SYSTEM_PROMPT }] },
        contents,
        generationConfig: { temperature: 0.7, maxOutputTokens: 1024 },
      }),
    })

    if (!res.ok) {
      throw new Error(`API error ${res.status}`)
    }

    const data = await res.json()
    const reply =
      data?.candidates?.[0]?.content?.parts?.map((p) => p.text).join('') ??
      '(sem resposta)'

    messages.value.push({ role: 'assistant', text: reply })
  } catch (err) {
    messages.value.push({
      role: 'assistant',
      text: 'Ups, algo falhou ao contactar o Gemini. Tenta novamente dentro de um momento.',
    })
  } finally {
    loading.value = false
    await scrollToBottom()
  }
}

function onKeydown(e) {
  if (e.key === 'Enter' && !e.shiftKey) {
    e.preventDefault()
    send()
  }
}

function escapeHtml(s) {
  return s
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
}

function formatMsg(text) {
  let html = escapeHtml(text)
  html = html.replace(
    /```(?:amethyst|amt|c\+\+|cpp|sh|bash)?\n?([\s\S]*?)```/g,
    (_, code) => `<pre class="ai-code">${code}</pre>`,
  )
  html = html.replace(/`([^`\n]+)`/g, '<code>$1</code>')
  html = html.replace(/\*\*([^*\n]+)\*\*/g, '<strong>$1</strong>')
  html = html.replace(/\n/g, '<br />')
  return html
}
</script>

<template>
  <div class="ai-assistant">
    <Transition name="pop">
      <div v-if="open" class="ai-panel" role="dialog" aria-label="Assistente de IA">
        <header class="ai-panel__header">
          <div class="ai-panel__title">
            <span class="ai-panel__avatar">◆</span>
            <div>
              <strong>Assistente Amethyst</strong>
              <span class="ai-panel__model">Gemini 3 Flash</span>
            </div>
          </div>
          <button class="ai-panel__close" aria-label="Fechar" @click="toggle">
            ×
          </button>
        </header>

        <div ref="messagesEl" class="ai-panel__messages">
          <div
            v-for="(m, i) in messages"
            :key="i"
            class="ai-msg"
            :class="m.role === 'user' ? 'ai-msg--user' : 'ai-msg--bot'"
          >
            <div class="ai-msg__bubble" v-html="formatMsg(m.text)"></div>
          </div>
          <div v-if="loading" class="ai-msg ai-msg--bot">
            <div class="ai-msg__bubble ai-msg__bubble--typing">
              <span></span><span></span><span></span>
            </div>
          </div>
        </div>

        <footer class="ai-panel__input">
          <input
            v-model="input"
            type="text"
            placeholder="Escreve a tua pergunta…"
            :disabled="loading"
            @keydown="onKeydown"
          />
          <button
            class="ai-panel__send"
            :disabled="loading || !input.trim()"
            aria-label="Enviar"
            @click="send"
          >
            →
          </button>
        </footer>
      </div>
    </Transition>

    <button
      class="ai-fab"
      :aria-label="open ? 'Fechar assistente' : 'Abrir assistente'"
      @click="toggle"
    >
      <span v-if="!open" class="ai-fab__icon">◆</span>
      <span v-else class="ai-fab__icon">×</span>
      <span v-if="!open" class="ai-fab__label">IA</span>
    </button>
  </div>
</template>

<style scoped>
.ai-assistant {
  position: fixed;
  right: 1.25rem;
  bottom: 1.25rem;
  z-index: 100;
  display: flex;
  flex-direction: column;
  align-items: flex-end;
  gap: 0.75rem;
}

.ai-fab {
  display: inline-flex;
  align-items: center;
  gap: 0.45rem;
  padding: 0.8rem 1.1rem;
  border-radius: 999px;
  border: 1px solid rgba(168, 85, 247, 0.5);
  background: linear-gradient(135deg, var(--amethyst) 0%, var(--amethyst-deep) 100%);
  color: #fff;
  font-weight: 700;
  font-size: 0.9rem;
  cursor: pointer;
  box-shadow: 0 6px 28px rgba(168, 85, 247, 0.45);
  transition: transform 0.15s ease, box-shadow 0.15s ease;
}

.ai-fab:hover {
  transform: translateY(-2px);
  box-shadow: 0 10px 36px rgba(168, 85, 247, 0.55);
}

.ai-fab__icon {
  font-size: 1rem;
  line-height: 1;
}

.ai-panel {
  width: min(380px, calc(100vw - 2rem));
  height: min(520px, calc(100vh - 7rem));
  display: flex;
  flex-direction: column;
  background: var(--bg-card);
  border: 1px solid var(--border-strong);
  border-radius: 16px;
  overflow: hidden;
  box-shadow: 0 24px 64px rgba(0, 0, 0, 0.55);
}

.ai-panel__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0.85rem 1rem;
  background: linear-gradient(135deg, rgba(168, 85, 247, 0.18), rgba(124, 58, 237, 0.08));
  border-bottom: 1px solid var(--border);
}

.ai-panel__title {
  display: flex;
  align-items: center;
  gap: 0.65rem;
}

.ai-panel__title strong {
  display: block;
  font-size: 0.92rem;
}

.ai-panel__avatar {
  width: 34px;
  height: 34px;
  border-radius: 50%;
  display: grid;
  place-items: center;
  background: linear-gradient(135deg, var(--amethyst), var(--amethyst-deep));
  color: #fff;
  font-size: 0.9rem;
}

.ai-panel__model {
  font-size: 0.72rem;
  font-family: var(--mono);
  color: var(--amethyst-bright);
}

.ai-panel__close {
  background: transparent;
  border: none;
  color: var(--text-dim);
  font-size: 1.4rem;
  line-height: 1;
  cursor: pointer;
  padding: 0.15rem 0.4rem;
}

.ai-panel__close:hover {
  color: var(--text);
}

.ai-panel__messages {
  flex: 1;
  overflow-y: auto;
  padding: 1rem;
  display: flex;
  flex-direction: column;
  gap: 0.7rem;
  background: var(--bg-soft);
}

.ai-msg {
  display: flex;
}

.ai-msg--user {
  justify-content: flex-end;
}

.ai-msg--bot {
  justify-content: flex-start;
}

.ai-msg__bubble {
  max-width: 85%;
  padding: 0.6rem 0.85rem;
  border-radius: 14px;
  font-size: 0.88rem;
  line-height: 1.5;
  color: var(--text);
  word-break: break-word;
}

.ai-msg--bot .ai-msg__bubble {
  background: var(--bg-card);
  border: 1px solid var(--border);
  border-bottom-left-radius: 4px;
}

.ai-msg--user .ai-msg__bubble {
  background: linear-gradient(135deg, var(--amethyst), var(--amethyst-deep));
  border-bottom-right-radius: 4px;
}

.ai-msg__bubble :deep(.ai-code) {
  margin: 0.5rem 0;
  padding: 0.6rem 0.75rem;
  background: #100c1c;
  border: 1px solid var(--border);
  border-radius: 8px;
  overflow-x: auto;
  font-family: var(--mono);
  font-size: 0.8rem;
  line-height: 1.5;
  white-space: pre;
}

.ai-msg__bubble :deep(code) {
  font-family: var(--mono);
  font-size: 0.84em;
  background: rgba(168, 85, 247, 0.15);
  color: var(--amethyst-bright);
  padding: 0.05rem 0.3rem;
  border-radius: 4px;
}

.ai-msg__bubble--typing {
  display: inline-flex;
  gap: 5px;
  align-items: center;
}

.ai-msg__bubble--typing span {
  width: 7px;
  height: 7px;
  border-radius: 50%;
  background: var(--amethyst-bright);
  animation: blink 1.2s infinite ease-in-out;
}

.ai-msg__bubble--typing span:nth-child(2) {
  animation-delay: 0.2s;
}

.ai-msg__bubble--typing span:nth-child(3) {
  animation-delay: 0.4s;
}

@keyframes blink {
  0%,
  80%,
  100% {
    opacity: 0.25;
    transform: scale(0.85);
  }
  40% {
    opacity: 1;
    transform: scale(1);
  }
}

.ai-panel__input {
  display: flex;
  gap: 0.5rem;
  padding: 0.75rem;
  border-top: 1px solid var(--border);
  background: var(--bg-card);
}

.ai-panel__input input {
  flex: 1;
  background: var(--bg);
  border: 1px solid var(--border);
  border-radius: 10px;
  padding: 0.6rem 0.8rem;
  color: var(--text);
  font-size: 0.88rem;
  font-family: var(--sans);
  outline: none;
}

.ai-panel__input input:focus {
  border-color: var(--amethyst);
}

.ai-panel__send {
  width: 40px;
  border: none;
  border-radius: 10px;
  background: linear-gradient(135deg, var(--amethyst), var(--amethyst-deep));
  color: #fff;
  font-size: 1.1rem;
  cursor: pointer;
  transition: opacity 0.15s;
}

.ai-panel__send:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.pop-enter-active,
.pop-leave-active {
  transition: opacity 0.18s ease, transform 0.18s ease;
}

.pop-enter-from,
.pop-leave-to {
  opacity: 0;
  transform: translateY(12px) scale(0.97);
}

@media (max-width: 480px) {
  .ai-panel {
    width: calc(100vw - 1.5rem);
    height: min(70vh, 480px);
  }

  .ai-assistant {
    right: 0.75rem;
    bottom: 0.75rem;
  }
}
</style>
