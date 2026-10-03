<script setup>
import CodeBlock from '../../components/CodeBlock.vue'

const path = [
  { step: '1', title: 'Instala o toolchain', text: 'Precisas de g++, as e ld (binutils + gcc). No Ubuntu/Debian: build-essential.' },
  { step: '2', title: 'Compila o compilador', text: 'Na raiz do repositório: make. Em segundos tens ./amethystc.' },
  { step: '3', title: 'Escreve o teu .amt', text: 'Cria um ficheiro hello.amt com um main() mínimo e um print.' },
  { step: '4', title: 'Compila e corre', text: './amethystc hello.amt -o hello && ./hello' },
  { step: '5', title: 'Explora os exemplos', text: 'examples/ tem fib, control, params — lê, altera, recompila.' },
  { step: '6', title: 'Faz o teu primeiro exercício', text: 'Troca o código dos exemplos por algo teu. Erra em propósito.' },
]

const engagement = [
  {
    icon: '⏱',
    title: 'Sessões curtas',
    text: '20–30 minutos > 3 horas esgotados. Fecha a sessão sabendo exatamente o que vais experimentar na próxima.',
  },
  {
    icon: '✎',
    title: 'Mão na massa sempre',
    text: 'Lê um conceito → escreve uma versão minúscula → quebra-a → conserta-a. Nada de só ler.',
  },
  {
    icon: '🐞',
    title: 'Lê os erros',
    text: 'file:linha:coluna é o teu mapa. Cada erro do Amethyst é uma aula gratuita de como o compilador pensa.',
  },
  {
    icon: '🔁',
    title: 'Reescreve, não copies',
    text: 'Depois de ver um exemplo, fecha-o e reescreve de memória. Se falhas num detalhe, esse detalhe era o teu gap.',
  },
  {
    icon: '🏁',
    title: 'Micro-projetos',
    text: 'Calculadora, jogo do adivinha, contador de palavras… algo que corre e podes mostrar em 1 dia.',
  },
  {
    icon: '📈',
    title: 'Diário de 3 linhas',
    text: 'No fim de cada sessão: o que aprendi / o que me confundiu / o que vou tentar amanhã.',
  },
]
</script>

<template>
  <div>
    <h1>For Beginners</h1>
    <p>
      Guia para quem nunca compilou uma linguagem própria — ou nunca programou
      em algo tipo C. Sem jargão grátis: só o que precisas para arrancar,
      <strong>sentir progresso</strong> e <strong>aguentar a curva</strong>
      até o Amethyst parecer teu.
    </p>

    <div class="callout">
      <strong>Regra de ouro:</strong> objectivo de cada sessão é
      <em>um programa que corre</em>, não “terminar os docs”. Docs são mapa;
      o código é a viagem.
    </div>

    <h2>Antes de começar</h2>
    <h3>O que é (e não é) Amethyst</h3>
    <ul>
      <li><strong>É</strong> compilada — o teu .amt vira binário nativo, sem intérprete.</li>
      <li><strong>É</strong> tipada — <code>int</code> e <code>bool</code> não se misturam.</li>
      <li><strong>É</strong> pequena de propósito — a base fica sólida antes de crescer.</li>
      <li><strong>Não é</strong> para escreveres uma app de produção amanhã.</li>
      <li><strong>Não é</strong> para teres medo de errar — errar é o modo de aprender.</li>
    </ul>

    <h3>Pré-requisitos mínimos</h3>
    <ul>
      <li>Confortável com terminal: <code>cd</code>, <code>ls</code>, correr um binário.</li>
      <li>Uma ideia vaga de variáveis e <code>if</code> (qualquer linguagem serve).</li>
      <li>Não precisas de saber C, assembly ou como liga o <code>ld</code>.</li>
    </ul>

    <h2>O teu primeiro dia (roadmap passo a passo)</h2>
    <ol class="beginner-path">
      <li v-for="item in path" :key="item.step">
        <span class="path-step">{{ item.step }}</span>
        <div>
          <strong>{{ item.title }}</strong>
          <p>{{ item.text }}</p>
        </div>
      </li>
    </ol>

    <h3>Instalação em 3 comandos (Dev Kit — sem compilar o compilador)</h3>
    <CodeBlock
      :code="`# na raiz do repo\nmake deb\nsudo apt install ./dist/amethyst-devkit_1.0.0_amd64.deb\n\n# primeiro programa\namethyst-new hello\namethystc hello.amt -o hello\n./hello   # 42`"
      filename="terminal"
    />
    <p>
      O <code>.deb</code> traz binário, templates, exemplos e
      <code>man</code>. Se preferires build from source:
      <router-link to="/docs/install">Instalação</router-link>.
    </p>

    <h3>O programa “olá mundo”</h3>
    <CodeBlock
      :code="`fn main() -> int {\n    print(42);\n    return 0;\n}`"
      filename="hello.amt"
    />
    <CodeBlock
      :code="`./amethystc hello.amt -o hello\n./hello\n# 42`"
      filename="terminal"
    />

    <div class="callout">
      <strong>Sanidade:</strong> se <code>make test</code> falhou, não é culpa
      do teu código — arranja o ambiente primeiro. Erros do teu .amt aparecem
      como <code>ficheito:linha:coluna: error: …</code>.
    </div>

    <h2>Plano de 7 dias (sugestão)</h2>
    <p>Um bocadinho por dia basta. Cada dia termina com algo a correr.</p>
    <table>
      <thead>
        <tr><th>Dia</th><th>Foco</th><th>Mini-projecto</th></tr>
      </thead>
      <tbody>
        <tr>
          <td>1</td>
          <td>Instalar, <code>main</code>, <code>print</code>, <code>return</code></td>
          <td>Imprime o teu número favorito + <code>true</code>/<code>false</code></td>
        </tr>
        <tr>
          <td>2</td>
          <td><code>var</code>, <code>int</code>, aritmética, atribuição</td>
          <td>Calcula área de um retângulo e imprime</td>
        </tr>
        <tr>
          <td>3</td>
          <td><code>bool</code>, comparações, <code>&&</code>/<code>||</code>/<code>!</code></td>
          <td>Classifica idade: maior de idade sim/não</td>
        </tr>
        <tr>
          <td>4</td>
          <td><code>if</code> / <code>else if</code> / <code>else</code></td>
          <td>Maior, menor ou igual — imprime o sinal</td>
        </tr>
        <tr>
          <td>5</td>
          <td><code>while</code> + contadores</td>
          <td>Conta de 1 a 10; depois só os pares</td>
        </tr>
        <tr>
          <td>6</td>
          <td>Funções, parâmetros, <code>return</code></td>
          <td><code>abs</code>, <code>max</code>, <code>dobro</code></td>
        </tr>
        <tr>
          <td>7</td>
          <td>Combina tudo + recursão</td>
          <td>Tabela de <code>fib</code> ou jogo do adivinha (vs teu valor fixo)</td>
        </tr>
      </tbody>
    </table>

    <h2>Exercícios para ganhares confiança</h2>
    <p>Começa pelos “fáceis” — se travares, voltas ao doc relevante e tentas sem copiar.</p>
    <ol>
      <li>Imprime <code>1 + 2 * 3</code> e depois <code>(1 + 2) * 3</code> — vê a diferença.</li>
      <li>Compara dois ints e imprime o resultado da comparação (<code>10 > 3</code>).</li>
      <li>Escreve <code>fn dobro(n: int) -> int</code> e usa-o.</li>
      <li>FizzBuzz só com <code>while</code> + <code>if</code>s (sem funções auxiliares).</li>
      <li>Soma os múltiplos de 3 ou 5 abaixo de 1000 (Project Euler #1).</li>
      <li>Fatorial iterativo; depois a versão recursiva — compara mentalmente.</li>
    </ol>

    <h2>Como te sentires engajado do início ao fim</h2>
    <p>
      Motivação não é magia: é engenharia de hábito. Usa o que funciona com
      a tua vida real, ignora o resto.
    </p>

    <div class="engage-grid">
      <article v-for="tip in engagement" :key="tip.title" class="engage-card">
        <div class="engage-icon">{{ tip.icon }}</div>
        <h3>{{ tip.title }}</h3>
        <p>{{ tip.text }}</p>
      </article>
    </div>

    <h3>Quando travares (e vais travar)</h3>
    <ol>
      <li><strong>Lê a linha inteira do erro</strong> — linha e coluna apontam para a causa.</li>
      <li><strong>Reduz</strong> — comenta metade do programa até o erro desaparecer.</li>
      <li><strong>Print debugging</strong> — <code>print</code> no meio é legítimo (e ensina).</li>
      <li><strong>Respira e dorme nele</strong> — alguns bugs só se revelam de manhã.</li>
      <li><strong>Pede / abre issue</strong> — se o compilador te enganou, é bug da Amethyst, não teu.</li>
    </ol>

    <h3>Clube de estudo (sozinho ou não)</h3>
    <ul>
      <li>Aprende com alguém — explicar em voz alta consolida 2× mais.</li>
      <li>Compara a tua solução com <code>examples/</code> só <em>depois</em> de teres uma.</li>
      <li>Um repositório “amethyst-exercises” teu, com commit por dia.</li>
      <li>Celebra micro-vitórias: “hoje o meu while parou no sítio certo” conta.</li>
    </ul>

    <h2>Onde continuar daqui</h2>
    <div class="next-grid">
      <router-link to="/docs/install" class="next-card">
        <span class="next-label">Próximo</span>
        <strong>Instalação</strong>
        <p>Detalhes do CLI e do toolchain.</p>
      </router-link>
      <router-link to="/docs/syntax" class="next-card">
        <span class="next-label">Depois</span>
        <strong>Sintaxe</strong>
        <p>Todo o esqueleto da linguagem.</p>
      </router-link>
      <router-link to="/docs/types" class="next-card">
        <span class="next-label">Fundamentos</span>
        <strong>Tipos</strong>
        <p>int, bool, arrays e as regras rígidas.</p>
      </router-link>
      <router-link to="/docs/statements" class="next-card">
        <span class="next-label">Prática</span>
        <strong>Statements</strong>
        <p>if, while, for, arrays em dia-a-dia.</p>
      </router-link>
    </div>

    <div class="callout">
      <strong>Checklist “estou a evoluir?”</strong>
      <ul style="margin: 0.5rem 0 0; padding-left: 1.1rem">
        <li>Consigo traduzir um problema em <code>var</code> + <code>while</code> + <code>print</code></li>
        <li>Leio um erro sem panicar</li>
        <li>Escrevo uma função com parâmetros sem olhar o cheat sheet</li>
        <li>Debugo com <code>print</code> de forma organizada</li>
        <li>Tenho 1 micro-projecto “meu”, não só dos exemplos</li>
      </ul>
    </div>
  </div>
</template>

<style scoped>
.beginner-path {
  list-style: none;
  margin: 1rem 0 1.5rem;
  padding: 0;
  display: flex;
  flex-direction: column;
  gap: 0.85rem;
}

.beginner-path li {
  display: grid;
  grid-template-columns: 36px 1fr;
  gap: 0.9rem;
  align-items: start;
  padding: 0.9rem 1rem;
  background: var(--bg-card);
  border: 1px solid var(--border);
  border-radius: var(--radius);
}

.path-step {
  width: 36px;
  height: 36px;
  border-radius: 50%;
  background: linear-gradient(135deg, var(--amethyst), var(--amethyst-deep));
  color: #fff;
  font-weight: 700;
  font-size: 0.9rem;
  display: grid;
  place-items: center;
}

.beginner-path strong {
  color: var(--text);
}

.beginner-path p {
  margin: 0.25rem 0 0;
  font-size: 0.92rem;
}

.engage-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 1rem;
  margin: 1.25rem 0 1.5rem;
}

.engage-card {
  background: var(--bg-card);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  padding: 1.15rem 1.1rem;
  transition: border-color 0.2s, transform 0.2s;
}

.engage-card:hover {
  border-color: var(--border-strong);
  transform: translateY(-2px);
}

.engage-icon {
  font-size: 1.3rem;
  margin-bottom: 0.4rem;
}

.engage-card h3 {
  margin: 0 0 0.4rem;
  font-size: 1rem;
}

.engage-card p {
  margin: 0;
  font-size: 0.9rem;
}

.next-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 0.85rem;
  margin: 1.25rem 0;
}

.next-card {
  display: block;
  padding: 1.1rem 1.15rem;
  background: var(--bg-card);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  text-decoration: none !important;
  color: inherit;
  transition: border-color 0.2s, transform 0.2s;
}

.next-card:hover {
  border-color: var(--amethyst);
  transform: translateY(-2px);
}

.next-label {
  font-family: var(--mono);
  font-size: 0.7rem;
  text-transform: uppercase;
  letter-spacing: 0.08em;
  color: var(--amethyst-bright);
}

.next-card strong {
  display: block;
  margin-top: 0.3rem;
  color: var(--text);
  font-size: 1.02rem;
}

.next-card p {
  margin: 0.3rem 0 0;
  font-size: 0.88rem;
}

@media (max-width: 640px) {
  .engage-grid,
  .next-grid {
    grid-template-columns: 1fr;
  }
}
</style>
