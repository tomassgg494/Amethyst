<script setup>
import CodeBlock from '../../components/CodeBlock.vue'
</script>

<template>
  <div>
    <h1>Statements</h1>
    <p>
      Statements terminam em <code>;</code> (blocos e statements de controlo
      com <code>{ }</code> não levam <code>;</code> extra no fim).
    </p>

    <h2>Declaração de variável</h2>
    <CodeBlock
      :code="`var x: int = 1;\nvar flag: bool = false;\nvar n = 42;          // tipo inferido\nvar a: int[3] = [1, 2, 3];\nvar b = [true, false]; // bool[2] inferido`"
      filename="var.amt"
    />

    <h2>Atribuição</h2>
    <CodeBlock :code="`x = x + 1;`" filename="assign.amt" />
    <p>
      A variável tem de existir no escopo atual (ou num escopo exterior) e o
      tipo do RHS tem de ser idêntico.
    </p>
    <p>
      Elementos de array atribuem-se por índice (não se pode atribuir ao
      array inteiro):
    </p>
    <CodeBlock :code="`nums[2] = 99;`" filename="assign-index.amt" />

    <h2>Bloco</h2>
    <p>
      <code>{ … }</code> abre um escopo novo. Variáveis declaradas dentro
      não são visíveis fora.
    </p>
    <CodeBlock
      :code="`{\n    var tmp: int = 5;\n    print(tmp);\n}`"
      filename="block.amt"
    />

    <h2>if / else</h2>
    <p>
      A condição tem de ser <code>bool</code> (sem <em>truthiness</em> de
      inteiros). <code>else</code> aceita bloco ou outro <code>if</code>
      (<code>else if</code>).
    </p>
    <CodeBlock
      :code="`if n < 0 {\n    print(-1);\n} else if n == 0 {\n    print(0);\n} else {\n    print(1);\n}`"
      filename="if.amt"
    />

    <h2>while</h2>
    <p>Reavalia a condição <code>bool</code> a cada iteração.</p>
    <CodeBlock
      :code="`var i: int = 0;\nwhile i < 5 {\n    print(i);\n    i = i + 1;\n}`"
      filename="while.amt"
    />

    <h2>for (range)</h2>
    <p>
      Itera sobre um range meio-aberto <code>[início, fim)</code> com
      passo <code>+1</code>. A variável do loop é <code>int</code> com
      escopo próprio (visível só no corpo). O valor final é avaliado
      <strong>uma vez</strong>, antes do loop.
    </p>
    <CodeBlock
      :code="`for i in 0..5 {\n    print(i);   // 0 1 2 3 4\n}`"
      filename="for.amt"
    />
    <CodeBlock
      :code="`// ranges dinâmicos\nfor i in 0..n {\n    ...\n}`"
      filename="for-dynamic.amt"
    />

    <h2>break / continue</h2>
    <p>
      Válidos <strong>só dentro de um loop</strong> (<code>while</code> ou
      <code>for</code>). <code>break</code> sai do loop mais interior;
      <code>continue</code> salta para a próxima iteração.
    </p>
    <CodeBlock
      :code="`for i in 0..10 {\n    if i == 3 {\n        continue;  // salta o 3\n    }\n    if i == 7 {\n        break;      // para em 7\n    }\n    print(i);\n}`"
      filename="break-continue.amt"
    />
    <p>
      Com aninhamento, cada <code>break</code>/<code>continue</code> afecta
      apenas o loop mais interior.
    </p>

    <h2>return</h2>
    <ul>
      <li>Função <code>void</code>: <code>return;</code> ou fim natural do bloco</li>
      <li>
        Função não-<code>void</code>: <code>return expr;</code> com tipo
        correto, e <strong>todos os caminhos</strong> de controlo têm de
        retornar (o sema verifica <code>return</code>, blocos e
        <code>if</code>/<code>else</code>; <code>while</code> sozinho não conta)
      </li>
    </ul>
    <CodeBlock
      :code="`fn abs(n: int) -> int {\n    if n < 0 {\n        return -n;\n    }\n    return n;\n}`"
      filename="abs.amt"
    />

    <h2>print</h2>
    <p>
      Statement embutido (não é função): imprime um <code>int</code>,
      <code>bool</code> ou <strong>literal de string</strong> com newline.
      Inteiros/bools usam <code>printf("%ld\\n", …)</code>; strings usam
      <code>puts</code>.
    </p>
    <CodeBlock
      :code='`print(42);\nprint(true);\nprint("Hello, Amethyst!");\nprint("tab:\\t quote:\\"");`'
      filename="print.amt"
    />
    <p>Escapes de string: <code>\n</code>, <code>\t</code>, <code>\"</code>, <code>\\</code>.</p>

    <h2>Expressão como statement</h2>
    <p>
      Chamadas a funções podem ser usadas como statement (o valor de retorno
      é descartado):
    </p>
    <CodeBlock
      :code="`helper();  // descarta o int devolvido`"
      filename="expr-stmt.amt"
    />

    <div class="callout">
      <strong>Escopo:</strong> redeclarar o mesmo nome no mesmo escopo é
      erro; sombras entre escopos diferentes ainda não são proibidas
      explicitamente na v1.1 além da checagem por escopo.
    </div>
  </div>
</template>
