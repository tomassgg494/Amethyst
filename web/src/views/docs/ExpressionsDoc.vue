<script setup>
import CodeBlock from '../../components/CodeBlock.vue'
</script>

<template>
  <div>
    <h1>Expressões</h1>
    <p>
      Toda expressão tem tipo e pode aparecer onde o contexto espera esse
      tipo (inicializador, condição, argumento, <code>return</code>, …).
    </p>

    <h2>Precedência (da mais baixa à mais alta)</h2>
    <table>
      <thead>
        <tr><th>#</th><th>Operadores</th><th>Notas</th></tr>
      </thead>
      <tbody>
        <tr><td>1</td><td><code>||</code></td><td>curto-circuito</td></tr>
        <tr><td>2</td><td><code>&&</code></td><td>curto-circuito</td></tr>
        <tr><td>3</td><td><code>== !=</code></td><td>associativo à esquerda</td></tr>
        <tr><td>4</td><td><code>&lt; &lt;= &gt; &gt;=</code></td><td>→ <code>bool</code></td></tr>
        <tr><td>5</td><td><code>+ -</code></td><td><code>int</code></td></tr>
        <tr><td>6</td><td><code>* / %</code></td><td><code>int</code></td></tr>
        <tr><td>7</td><td>unários <code>- !</code></td><td>direita-a-esquerda</td></tr>
        <tr><td>8</td><td><code>a[i]</code> (index)</td><td>após o primário</td></tr>
        <tr><td>9</td><td>literais, ident, chamada, <code>[…]</code>, <code>(…)</code></td><td>primários</td></tr>
      </tbody>
    </table>

    <h2>Literais</h2>
    <CodeBlock
      :code='`42          // int\ntrue        // bool\nfalse       // bool\n"hello"     // string (só com print)\n[1, 2, 3]   // int[3]\n[true, !false] // bool[2]`'
      filename="literals"
    />
    <p>Inteiros decimais não negativos; o menos é operador unário.</p>

    <h2>Operadores aritméticos</h2>
    <CodeBlock
      :code="`var a: int = 7 + 3 * 2;   // 13\nvar b: int = (7 + 3) * 2; // 20\nvar c: int = 7 % 3;       // 1\nvar d: int = -7 / 2;      // -3 (trunca p/ zero)`"
      filename="arith.amt"
    />

    <h2>Comparações</h2>
    <CodeBlock
      :code="`var t: bool = 3 < 4;     // true\nvar f: bool = 3 != 4;    // true\nvar e: bool = (1 == 1);  // true`"
      filename="cmp.amt"
    />
    <p>
      <code>==</code> / <code>!=</code> só entre tipos iguais (não
      <code>void</code>).
    </p>

    <h2>Lógicos com curto-circuito</h2>
    <CodeBlock
      :code="`// se lhs for false, rhs NÃO é avaliado\nvar a: bool = false && sideEffect();\n\n// se lhs for true, rhs NÃO é avaliado\nvar b: bool = true || sideEffect();`"
      filename="logic.amt"
    />

    <h2>Unários</h2>
    <CodeBlock
      :code="`var n: int = -5;\nvar flag: bool = !true;  // false`"
      filename="unary.amt"
    />

    <h2>Chamadas</h2>
    <CodeBlock
      :code="`fn add(a: int, b: int) -> int {\n    return a + b;\n}\n\nfn main() -> int {\n    var s: int = add(2, add(3, 4));  // 9\n    print(s);\n    return 0;\n}`"
      filename="calls.amt"
    />
    <ul>
      <li>Arity e tipos dos argumentos são verificados na análise semântica</li>
      <li>Recursão é suportada (<code>fib</code> clássico funciona)</li>
      <li>Chamar <code>print</code> como função é erro — é um statement</li>
    </ul>

    <h2>Index de array</h2>
    <CodeBlock
      :code="`var nums = [10, 20, 30];\nvar x: int = nums[1];  // 20\nnums[0] = 5;           // escrita como statement`"
      filename="index.amt"
    />
    <p>
      O índice tem de ser <code>int</code>. <strong>Bounds check em
      runtime:</strong> fora de <code>[0, N)</code> → mensagem de erro e
      exit code 1.
    </p>
    <div class="callout">
      <strong>Nota:</strong> <code>nums[0] = 5;</code> é um <em>statement</em>
      de atribuição indexada, não uma expressão avaliada.
    </div>

    <h2>Strings</h2>
    <p>
      Literais entre aspas duplas, válidas <strong>apenas</strong> como
      argumento de <code>print</code> na v1.1:
    </p>
    <CodeBlock
      :code='`print("olá");\nprint("a\\tb\\n");`'
      filename="strings.amt"
    />
    <p>
      Escapes: <code>\n</code> (newline), <code>\t</code> (tab),
      <code>\"</code> (aspas), <code>\\</code> (barra). Não se pode
      guardar numa variável nem comparar.
    </p>

    <h2>Parênteses</h2>
    <p>
      Use <code>(…)</code> livremente para agrupar; precedência sozinha já
      resolve os casos comuns.
    </p>
  </div>
</template>
