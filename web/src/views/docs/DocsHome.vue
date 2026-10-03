<script setup>
import CodeBlock from '../../components/CodeBlock.vue'

const sample = `fn main() -> int {
    var message: int = 42;
    print(message);
    return 0;
}`
</script>

<template>
  <div>
    <h1>Introdução</h1>
    <p>
      <strong>Amethyst</strong> é uma linguagem <em>compilada</em> com foco em
      velocidade e previsibilidade. A sintaxe fica a meio caminho entre
      linguagens high-level (Python, JavaScript) e low-level (C, C++):
      chaves, ponto-e-vírgula, tipos explícitos, sem magia.
    </p>

    <div class="callout">
      <strong>v1.1 atual:</strong> <code>int</code>, <code>bool</code>,
      arrays <code>int[N]</code>/<code>bool[N]</code> (com bounds check),
      strings em <code>print</code>, funções, <code>if</code>,
      <code>while</code>, <code>for</code> ranges, <code>break</code>/<code>continue</code>,
      variáveis com inferência e <code>print</code>. Compilador em C++17,
      backend GAS x86-64.
    </div>

    <h2>Olá, Amethyst</h2>
    <CodeBlock :code="sample" filename="hello.amt" />

    <h2>Caraterísticas</h2>
    <ul>
      <li><strong>Compilada</strong> — pipeline <code>.amt → .s → .o → binário</code></li>
      <li><strong>Estática</strong> — erros de tipo e de caminhos de retorno em compile-time</li>
      <li><strong>Rápida</strong> — código nativo, sem camada de interpretação</li>
      <li><strong>Legível</strong> — sintaxe minimalista mas familiar</li>
      <li><strong>Sem dependências pesadas</strong> — só <code>g++</code>, <code>as</code> e <code>ld</code></li>
    </ul>

    <h2>Próximos passos</h2>
    <ol>
      <li>
        <router-link to="/docs/beginners">For Beginners</router-link> — se
        estás a começar, vai direto aqui
      </li>
      <li><router-link to="/docs/install">Instalação</router-link> — build em segundos</li>
      <li><router-link to="/docs/syntax">Sintaxe</router-link> — visão geral da linguagem</li>
      <li><router-link to="/docs/types">Tipos</router-link> — <code>int</code>, <code>bool</code>, arrays, strings</li>
      <li><router-link to="/docs/statements">Statements</router-link> e <router-link to="/docs/expressions">expressões</router-link></li>
      <li><router-link to="/docs/compiler">Compilador</router-link> — como funciona por baixo</li>
    </ol>
  </div>
</template>
