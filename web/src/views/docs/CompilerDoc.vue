<script setup>
import CodeBlock from '../../components/CodeBlock.vue'
</script>

<template>
  <div>
    <h1>Compilador</h1>
    <p>
      <code>amethystc</code> é um programa C++17 sem LLVM: analisa a fonte,
      emite assembly GAS x86-64 e usa o toolchain do sistema para montar e
      ligar.
    </p>

    <h2>Pipeline</h2>
    <CodeBlock
      :code="`file.amt\n  │  Lexer      → tokens\n  │  Parser     → AST\n  │  Sema       → tipos, escopos, frame slots\n  │  Codegen    → file.s  (GAS, System V AMD64)\n  │  as --64    → file.o\n  └  gcc -no-pie→ executável  (ld + crt + libc)`"
      filename="pipeline"
    />

    <h2>Fontes</h2>
    <table>
      <thead>
        <tr><th>Ficheiro</th><th>Responsabilidade</th></tr>
      </thead>
      <tbody>
        <tr><td><code>src/token.hpp</code></td><td>tipos de token</td></tr>
        <tr><td><code>src/lexer.*</code></td><td>fonte → tokens, erros linha:coluna</td></tr>
        <tr><td><code>src/ast.hpp</code></td><td>nós de expressão/statement/função</td></tr>
        <tr><td><code>src/parser.*</code></td><td>recursive descent + precedência</td></tr>
        <tr><td><code>src/sema.*</code></td><td>tabela de símbolos, tipos, slots de frame</td></tr>
        <tr><td><code>src/codegen.*</code></td><td>AST → assembly</td></tr>
        <tr><td><code>src/main.cpp</code></td><td>driver + invocação de <code>as</code>/<code>gcc</code></td></tr>
      </tbody>
    </table>

    <h2>Convencional de chamada (System V AMD64)</h2>
    <ul>
      <li>Argumentos inteiros: <code>rdi rsi rdx rcx r8 r9</code>, depois stack</li>
      <li>Retorno em <code>rax</code></li>
      <li>Frame pointer <code>rbp</code>; locais em <code>-8(%rbp)</code>, <code>-16(%rbp)</code>, …</li>
      <li>Parâmetros são “spilled” no prólogo para endereçamento uniforme</li>
      <li>Stack alinhada a 16 bytes antes de <code>call</code> (inclusive com aninhamento)</li>
    </ul>

    <h2>Exemplo de assembly gerada</h2>
    <CodeBlock
      :code="`.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movq $42, %rax
    leaq .fmt_int(%rip), %rdi
    movq %rax, %rsi
    xorl %eax, %eax
    call printf@PLT
    movq $0, %rax
    leave
    ret`"
      filename="main.s (excerpt)"
    />

    <h2>Análise semântica</h2>
    <ul>
      <li>Recolhe assinaturas de todas as funções primeiro (permite recursão mútua)</li>
      <li>Exige <code>main() -&gt; int</code> sem parâmetros</li>
      <li>Checagem de tipos em cada expressão/atribuição/condição</li>
      <li>Arity de chamadas e redeclarações</li>
      <li>
        Funções não-<code>void</code> têm de retornar em todos os caminhos
        (<code>return</code>, blocos, <code>if</code>/<code>else</code>)
      </li>
      <li>Atribui <code>frame slot</code> a cada <code>var</code>/parâmetro — o codegen não refaz lookup</li>
    </ul>

    <h2>Erros</h2>
    <p>Sempre no padrão de ferramentas Unix:</p>
    <CodeBlock
      :code="`prog.amt:4:12: error: cannot initialize 'int x' with value of type 'bool'`"
      filename="stderr"
    />

    <div class="callout">
      <strong>Extensibilidade:</strong> adicionar um statement ou tipo
      normalmente toca <code>parser</code> → <code>sema</code> →
      <code>codegen</code> nesse ordem, mais um exemplo em
      <code>examples/</code> e um teste.
    </div>
  </div>
</template>
