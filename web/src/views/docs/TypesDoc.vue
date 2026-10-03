<script setup>
import CodeBlock from '../../components/CodeBlock.vue'
</script>

<template>
  <div>
    <h1>Tipos</h1>
    <p>
      A v1.1 é pequena de propósito: dois tipos escalares, arrays de tamanho
      fixo e strings literais. <strong>Não há conversões implícitas</strong>
      entre <code>int</code> e <code>bool</code>.
    </p>

    <h2>int</h2>
    <p>
      Inteiro com sinal de 64 bits (<code>i64</code>), representado em
      registradores <code>rax</code> e na stack com 8 bytes.
    </p>
    <CodeBlock
      :code="`var n: int = 42;\nvar m: int = -7;\nvar k: int = n + m * 2;`"
      filename="ints.amt"
    />
    <p>Suporta aritmética <code>+ - * / %</code> com sinal (divisão truncada).</p>

    <h2>bool</h2>
    <p>
      <code>true</code> ou <code>false</code>. Na stack é guardado como 8 bytes
      (<code>0</code>/<code>1</code>); <code>print</code> mostra <code>1</code>
      ou <code>0</code>.
    </p>
    <CodeBlock
      :code="`var ok: bool = true;\nvar big: bool = 10 > 3;\nvar both: bool = ok && big;`"
      filename="bools.amt"
    />

    <h2>void</h2>
    <p>
      Só como tipo de retorno de funções sem valor. Não pode ser usado em
      <code>var</code>, nem em expressões.
    </p>
    <CodeBlock
      :code="`fn log() -> void {\n    print(1);\n}`"
      filename="void.amt"
    />

    <h2>Arrays — int[N] / bool[N]</h2>
    <p>
      Arrays de tamanho fixo, alocados na stack. O tamanho <code>N</code> é
      um literal inteiro (<code>1</code> a <code>10 000 000</code>). O
      inicializador é um literal de array com exactamente <code>N</code>
      elementos do tipo do elemento.
    </p>
    <CodeBlock
      :code="`var nums: int[5] = [10, 20, 30, 40, 50];\nvar flags: bool[3] = [true, false, true];\n\nprint(nums[0]);   // 10\nnums[2] = 99;     // escrita por índice`"
      filename="arrays.amt"
    />
    <ul>
      <li>Leitura <code>a[i]</code> e escrita <code>a[i] = expr;</code></li>
      <li>
        <strong>Bounds check em runtime:</strong> índice fora de
        <code>[0, N)</code> imprime mensagem de erro e sai com código 1
      </li>
      <li>Não se pode atribuir ao array inteiro, comparar arrays nem passá-los a funções (ainda)</li>
    </ul>

    <h2>Strings (literais)</h2>
    <p>
      Literais entre aspas duplas com escapes <code>\n</code>,
      <code>\t</code>, <code>&quot;</code> e <code>\\</code>. Na v1.1 só podem
      ser usados directamente com <code>print</code> — não há variáveis de
      tipo string nem comparações de strings.
    </p>
    <CodeBlock
      :code='`print("Hello!");\nprint("line1\\nline2");`'
      filename="strings.amt"
    />

    <h2>Regras de tipagem</h2>
    <table>
      <thead>
        <tr><th>Operação</th><th>Operandos</th><th>Resultado</th></tr>
      </thead>
      <tbody>
        <tr>
          <td><code>+ - * / %</code></td>
          <td><code>int</code>, <code>int</code></td>
          <td><code>int</code></td>
        </tr>
        <tr>
          <td><code>&lt; &lt;= &gt; &gt;=</code></td>
          <td><code>int</code>, <code>int</code></td>
          <td><code>bool</code></td>
        </tr>
        <tr>
          <td><code>== !=</code></td>
          <td>mesmo tipo escalar (<code>int</code>/<code>bool</code>)</td>
          <td><code>bool</code></td>
        </tr>
        <tr>
          <td><code>&& ||</code></td>
          <td><code>bool</code>, <code>bool</code></td>
          <td><code>bool</code></td>
        </tr>
        <tr>
          <td><code>!</code></td>
          <td><code>bool</code></td>
          <td><code>bool</code></td>
        </tr>
        <tr>
          <td>unário <code>-</code></td>
          <td><code>int</code></td>
          <td><code>int</code></td>
        </tr>
      </tbody>
    </table>

    <div class="callout">
      <strong>Exemplo de erro:</strong>
      <code>var x: int = true;</code> falha com
      <code>cannot initialize 'int x' with value of type 'bool'</code>.
    </div>

    <h2>Declaração de variáveis</h2>
    <p>
      <code>var</code> exige inicializador — não há valores por omissão nem
      <em>definite assignment</em>. O tipo é <strong>opcional</strong>:
      quando omitido, é inferido do inicializador.
    </p>
    <CodeBlock
      :code="`var idade: int = 30;\nvar ativo: bool = idade >= 18;\n\n// inferência de tipo\nvar n = 42;             // int\nvar ok = n > 40;        // bool\nvar arr = [1, 2, 3];    // int[3]`"
      filename="decl.amt"
    />
    <p>
      Atribuição posterior usa <code>=</code> e o tipo tem de coincidir com
      o da declaração.
    </p>
    <div class="callout">
      <strong>Não dá para inferir:</strong> <code>var x = "hi";</code> —
      strings não são armazenáveis em variáveis (v1.1).
    </div>
  </div>
</template>
