<script setup>
import CodeBlock from '../../components/CodeBlock.vue'

const program = `// Comentário de linha
fn add(a: int, b: int) -> int {
    return a + b;
}

fn main() -> int {
    var nums = [10, 20, 30];  // tipo inferido: int[3]
    var ok: bool = nums[0] > 5;

    for i in 0..3 {
        if nums[i] % 2 != 0 {
            continue;
        }
        print(nums[i]);
    }

    if ok {
        print(add(nums[0], 32));
    } else {
        print(0);
    }

    print("done");
    return 0;
}`
</script>

<template>
  <div>
    <h1>Sintaxe</h1>
    <p>
      Amethyst usa <strong>chaves</strong> para blocos e
      <strong>ponto-e-vírgula</strong> para terminar statements — o mesmo
      esqueleto de C/JS/Java, com keywords curtas e tipos explícitos.
    </p>

    <CodeBlock :code="program" filename="overview.amt" />

    <h2>Estrutura de um programa</h2>
    <p>
      Um ficheiro é uma sequência de declarações de função. O ponto de entrada
      é obrigatoriamente:
    </p>
    <CodeBlock :code="`fn main() -> int { ... }`" filename="entry" />
    <ul>
      <li>sem parâmetros</li>
      <li>retorna <code>int</code> (o exit code do processo)</li>
    </ul>

    <h2>Funções</h2>
    <CodeBlock
      :code="`fn nome(p1: tipo, p2: tipo) -> tipoRetorno {\n    // corpo\n    return valor;\n}`"
      filename="fn"
    />
    <p>
      Parâmetros usam <code>nome: tipo</code>. O operador <code>-&gt;</code>
      liga a assinatura ao corpo. Até 6 parâmetros vão por registo (SysV);
      acima disso, pela stack.
    </p>

    <h2>Identificadores</h2>
    <ul>
      <li>Começam por letra ou <code>_</code></li>
      <li>Seguidos de letras, dígitos ou <code>_</code></li>
      <li>Sensíveis a maiúsculas/minúsculas</li>
    </ul>

    <h2>Keywords</h2>
    <table>
      <thead>
        <tr><th>Keyword</th><th>Uso</th></tr>
      </thead>
      <tbody>
        <tr><td><code>fn</code></td><td>declaração de função</td></tr>
        <tr><td><code>var</code></td><td>variável local com inicialização (tipo opcional)</td></tr>
        <tr><td><code>return</code></td><td>devolver valor (ou sair)</td></tr>
        <tr><td><code>if</code> / <code>else</code></td><td>condição (<code>bool</code>)</td></tr>
        <tr><td><code>while</code></td><td>loop (<code>bool</code>)</td></tr>
        <tr><td><code>for</code> … <code>in</code></td><td>loop em range: <code>for i in 0..10</code></td></tr>
        <tr><td><code>break</code></td><td>sair do loop atual</td></tr>
        <tr><td><code>continue</code></td><td>próxima iteração do loop atual</td></tr>
        <tr><td><code>print</code></td><td>imprimir <code>int</code>, <code>bool</code> ou string</td></tr>
        <tr><td><code>int</code> / <code>bool</code> / <code>void</code></td><td>tipos</td></tr>
        <tr><td><code>true</code> / <code>false</code></td><td>literais bool</td></tr>
      </tbody>
    </table>

    <h2>Comentários</h2>
    <CodeBlock :code="`// até o fim da linha`" filename="comment" />

    <div class="callout">
      <strong>Sem indentação semântica:</strong> espaços e tabs só alinham.
      O layout é sempre <code>{ }</code>.
    </div>
  </div>
</template>
