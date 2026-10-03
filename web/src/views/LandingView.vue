<script setup>
import CodeBlock from '../components/CodeBlock.vue'
import BenchmarksSection from '../components/BenchmarksSection.vue'

const sample = `fn fib(n: int) -> int {
    if n < 2 {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

fn main() -> int {
    var fibs = [0, 0, 0, 0, 0, 0];  // tipo inferido: int[6]

    for i in 0..6 {
        fibs[i] = fib(i);
    }
    for i in 0..6 {
        print(fibs[i]);
    }

    print("done");
    return 0;
}`
</script>

<template>
  <div class="landing">
    <!-- HERO -->
    <section class="hero">
      <div class="hero__glow" aria-hidden="true"></div>
      <div class="container hero__inner">
        <div class="hero__copy">
          <p class="eyebrow">linguagem compilada · x86-64 nativo</p>
          <h1>
            Rápida como <span class="grad">C</span>.<br />
            Legível como <span class="grad">Python</span>.<br />
            <span class="grad">Amethyst</span>.
          </h1>
          <p class="lead">
            Uma linguagem compilada com sintaxe a meio caminho entre
            high-level e low-level. Tipos estáticos, chaves e ponto-e-vírgula,
            zero de interpretação — assembly GAS, <code>as</code> e
            <code>ld</code>.
          </p>
          <div class="hero__actions">
            <router-link to="/docs" class="btn btn-primary">
              Ler a documentação →
            </router-link>
            <a href="#exemplo" class="btn btn-ghost">Ver código</a>
          </div>
          <div class="hero__meta">
            <span>C++17 compiler</span>
            <span class="dot"></span>
            <span>System V AMD64</span>
            <span class="dot"></span>
            <span>sem LLVM</span>
          </div>
        </div>

        <div class="hero__code" id="exemplo">
          <CodeBlock :code="sample" filename="fib.amt" />
        </div>
      </div>
    </section>

    <!-- FEATURES -->
    <section class="features">
      <div class="container">
        <h2 class="section-title">Porquê Amethyst?</h2>
        <p class="section-sub">
          Base sólida, escopo deliberadamente simples, zero magia.
        </p>

        <div class="features__grid">
          <article class="feature">
            <div class="feature__icon">⚡</div>
            <h3>Compilada de verdade</h3>
            <p>
              Emite assembly x86-64 GAS, monta com <code>as</code> e liga com
              o linker do sistema. Binário nativo, sem VM, sem intérprete.
            </p>
          </article>

          <article class="feature">
            <div class="feature__icon">◆</div>
            <h3>Sintaxe equilibrada</h3>
            <p>
              <code>fn</code>, <code>var</code>, chaves e <code>;</code> —
              familiar para quem vem de C/JS/Python, sem ruído desnecessário.
            </p>
          </article>

          <article class="feature">
            <div class="feature__icon">⎇</div>
            <h3>Tipos estáticos rígidos</h3>
            <p>
              <code>int</code>, <code>bool</code> e arrays nunca se misturam.
              Erros de tipo, arity e caminhos de <code>return</code> falham em
              compile-time — com inferência opcional em <code>var</code>.
            </p>
          </article>

          <article class="feature">
            <div class="feature__icon">⧉</div>
            <h3>Compilador em C++</h3>
            <p>
              Lexer, parser, semântica e codegen — tudo escrito em C++17,
              legível e fácil de estender.
            </p>
          </article>

          <article class="feature">
            <div class="feature__icon">⇄</div>
            <h3>Curto-circuito real</h3>
            <p>
              <code>&&</code> e <code>||</code> avaliam à esquerda e saltam
              o operando direito quando já decidido.
            </p>
          </article>

          <article class="feature">
            <div class="feature__icon">◎</div>
            <h3>Erros com posição</h3>
            <p>
              Toda mensagem no formato
              <code>file.amt:linha:coluna: error: …</code> — pronta para
              editores e CI.
            </p>
          </article>
        </div>
      </div>
    </section>

    <!-- BENCHMARKS -->
    <BenchmarksSection />

    <!-- PIPELINE -->
    <section class="pipeline">
      <div class="container">
        <h2 class="section-title">Pipeline de compilação</h2>
        <p class="section-sub">Seis passos até o executável.</p>

        <ol class="pipeline__list">
          <li><span class="step">1</span><strong>.amt</strong><span>fonte Amethyst</span></li>
          <li><span class="step">2</span><strong>Lexer</strong><span>tokens</span></li>
          <li><span class="step">3</span><strong>Parser</strong><span>AST</span></li>
          <li><span class="step">4</span><strong>Sema</strong><span>tipos &amp; slots</span></li>
          <li><span class="step">5</span><strong>Codegen</strong><span>GAS x86-64</span></li>
          <li><span class="step">6</span><strong>as · ld</strong><span>binário nativo</span></li>
        </ol>
      </div>
    </section>

    <!-- CTA -->
    <section class="cta">
      <div class="container cta__inner">
        <div>
          <h2>Começa agora</h2>
          <p>Compila, corre e testa em menos de um minuto.</p>
        </div>
        <div class="cta__actions">
          <router-link to="/docs/install" class="btn btn-primary">
            Instalação
          </router-link>
          <router-link to="/docs" class="btn btn-ghost">
            Documentação
          </router-link>
        </div>
      </div>
      <div class="container">
        <CodeBlock
          class="cta__cmd"
          :code="`make\nmake test\n./amethystc prog.amt -o prog\n./prog`"
          filename="terminal"
        />
      </div>
    </section>
  </div>
</template>

<style scoped>
.landing {
  overflow: hidden;
}

/* HERO */
.hero {
  position: relative;
  padding: 5.5rem 0 4.5rem;
}

.hero__glow {
  position: absolute;
  inset: -20% -10% auto;
  height: 70%;
  background:
    radial-gradient(ellipse 50% 60% at 30% 20%, rgba(168, 85, 247, 0.28), transparent 60%),
    radial-gradient(ellipse 40% 50% at 75% 10%, rgba(103, 232, 249, 0.1), transparent 55%);
  pointer-events: none;
}

.hero__inner {
  position: relative;
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 3rem;
  align-items: center;
}

.eyebrow {
  display: inline-block;
  font-family: var(--mono);
  font-size: 0.78rem;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: var(--amethyst-bright);
  background: rgba(168, 85, 247, 0.12);
  border: 1px solid rgba(168, 85, 247, 0.35);
  padding: 0.35rem 0.75rem;
  border-radius: 999px;
  margin: 0 0 1.25rem;
}

.hero h1 {
  font-size: clamp(2.2rem, 4.5vw, 3.4rem);
  margin: 0 0 1.25rem;
}

.grad {
  background: linear-gradient(120deg, var(--amethyst-bright), var(--cyan));
  -webkit-background-clip: text;
  background-clip: text;
  color: transparent;
}

.lead {
  font-size: 1.08rem;
  max-width: 34rem;
  margin: 0 0 1.75rem;
}

.lead code {
  color: var(--amethyst-bright);
  font-size: 0.92em;
}

.hero__actions {
  display: flex;
  flex-wrap: wrap;
  gap: 0.75rem;
  margin-bottom: 1.75rem;
}

.hero__meta {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 0.65rem;
  font-family: var(--mono);
  font-size: 0.78rem;
  color: var(--text-faint);
}

.hero__meta .dot {
  width: 4px;
  height: 4px;
  border-radius: 50%;
  background: var(--border-strong);
}

/* FEATURES */
.features {
  padding: 4.5rem 0;
  background: var(--bg-soft);
  border-block: 1px solid var(--border);
}

.section-title {
  text-align: center;
  font-size: clamp(1.6rem, 3vw, 2.1rem);
  margin: 0 0 0.5rem;
}

.section-sub {
  text-align: center;
  margin: 0 0 2.75rem;
  color: var(--text-faint);
}

.features__grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 1.25rem;
}

.feature {
  background: var(--bg-card);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  padding: 1.5rem 1.35rem;
  transition: border-color 0.2s, transform 0.2s;
}

.feature:hover {
  border-color: var(--border-strong);
  transform: translateY(-3px);
}

.feature__icon {
  font-size: 1.4rem;
  color: var(--amethyst-bright);
  margin-bottom: 0.75rem;
}

.feature h3 {
  margin: 0 0 0.5rem;
  font-size: 1.05rem;
}

.feature p {
  margin: 0;
  font-size: 0.92rem;
}

.feature code {
  color: var(--amethyst-bright);
  font-size: 0.88em;
}

/* PIPELINE */
.pipeline {
  padding: 4.5rem 0;
}

.pipeline__list {
  list-style: none;
  margin: 0;
  padding: 0;
  display: grid;
  grid-template-columns: repeat(6, 1fr);
  gap: 0.75rem;
}

.pipeline__list li {
  background: var(--bg-card);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  padding: 1.1rem 0.85rem;
  display: flex;
  flex-direction: column;
  gap: 0.35rem;
  text-align: center;
}

.pipeline__list .step {
  width: 26px;
  height: 26px;
  margin: 0 auto 0.35rem;
  border-radius: 50%;
  background: linear-gradient(135deg, var(--amethyst), var(--amethyst-deep));
  color: #fff;
  font-size: 0.75rem;
  font-weight: 700;
  display: grid;
  place-items: center;
}

.pipeline__list strong {
  font-size: 0.92rem;
}

.pipeline__list span:last-child {
  font-size: 0.75rem;
  color: var(--text-faint);
  font-family: var(--mono);
}

/* CTA */
.cta {
  padding: 1rem 0 4rem;
}

.cta__inner {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  justify-content: space-between;
  gap: 1.5rem;
  margin-bottom: 1.75rem;
}

.cta h2 {
  margin: 0 0 0.35rem;
}

.cta p {
  margin: 0;
}

.cta__actions {
  display: flex;
  gap: 0.75rem;
  flex-wrap: wrap;
}

.cta__cmd {
  max-width: 640px;
}

@media (max-width: 960px) {
  .hero__inner {
    grid-template-columns: 1fr;
  }

  .features__grid {
    grid-template-columns: 1fr 1fr;
  }

  .pipeline__list {
    grid-template-columns: repeat(3, 1fr);
  }
}

@media (max-width: 640px) {
  .features__grid {
    grid-template-columns: 1fr;
  }

  .pipeline__list {
    grid-template-columns: 1fr 1fr;
  }

  .hero {
    padding-top: 3rem;
  }
}
</style>
