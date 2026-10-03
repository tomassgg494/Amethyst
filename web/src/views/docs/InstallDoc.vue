<script setup>
import CodeBlock from '../../components/CodeBlock.vue'
</script>

<template>
  <div>
    <h1>Instalação</h1>
    <p>
      Para iniciantes, o caminho mais curto é o <strong>Dev Kit</strong>:
      um <code>.deb</code> com o compilador já built, templates, exemplos e
      man pages — <strong>sem compilar nada à mão</strong>.
    </p>

    <h2>Dev Kit (.deb) — recomendado</h2>
    <p>Na raiz do repositório:</p>
    <CodeBlock
      :code="`make deb\n# gera dist/amethyst-devkit_1.0.0_amd64.deb (~60 KB)\n\nsudo apt install ./dist/amethyst-devkit_1.0.0_amd64.deb`"
      filename="terminal"
    />

    <p>Depois de instalar, primeiro programa em 3 comandos:</p>
    <CodeBlock
      :code="`amethyst-new hello\namethystc hello.amt -o hello\n./hello   # 42`"
      filename="terminal"
    />

    <table>
      <thead>
        <tr><th>O que o pacote instala</th><th>Path</th></tr>
      </thead>
      <tbody>
        <tr><td>Compilador</td><td><code>/usr/bin/amethystc</code></td></tr>
        <tr><td>Helper de projecto</td><td><code>/usr/bin/amethyst-new</code></td></tr>
        <tr><td>Exemplos</td><td><code>/usr/share/amethyst/examples/</code></td></tr>
        <tr><td>Templates</td><td><code>/usr/share/amethyst/templates/</code></td></tr>
        <tr><td>Docs</td><td><code>/usr/share/doc/amethyst/README.md</code></td></tr>
        <tr><td>Man pages</td><td><code>man amethystc</code></td></tr>
      </tbody>
    </table>

    <div class="callout">
      <strong>Depends:</strong> <code>gcc</code> e <code>binutils</code> —
      o <code>amethystc</code> chama <code>as</code>/<code>ld</code> quando
      compila os <em>teus</em> programas. O apt trata disso.
    </div>

    <h2>Requisitos (build from source)</h2>
    <table>
      <thead>
        <tr>
          <th>Ferramenta</th>
          <th>Para quê</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <td><code>g++</code> (C++17)</td>
          <td>compilar o próprio compilador</td>
        </tr>
        <tr>
          <td><code>as</code> / <code>ld</code></td>
          <td>montar e ligar o <code>.s</code> gerado</td>
        </tr>
        <tr>
          <td><code>gcc</code></td>
          <td>linkar com crt + libc (<code>printf</code>)</td>
        </tr>
      </tbody>
    </table>

    <h2>Build from source</h2>
    <CodeBlock
      :code="`make          # gera ./amethystc\nmake test     # exemplos + suíte`"
      filename="terminal"
    />
    <p>Limpeza:</p>
    <CodeBlock :code="`make clean`" filename="terminal" />

    <h2>Primeiro programa</h2>
    <CodeBlock
      :code="`fn main() -> int {\n    print(42);\n    return 0;\n}`"
      filename="hello.amt"
    />
    <CodeBlock
      :code="`./amethystc hello.amt -o hello\n./hello\n# 42`"
      filename="terminal"
    />

    <h2>Opções do CLI</h2>
    <table>
      <thead>
        <tr>
          <th>Opção</th>
          <th>Efeito</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <td><code>-o &lt;path&gt;</code></td>
          <td>ficheiro de saída (default <code>a.out</code>)</td>
        </tr>
        <tr>
          <td><code>-S</code></td>
          <td>só emite assembly (<code>.s</code>), não monta nem liga</td>
        </tr>
        <tr>
          <td><code>--emit-asm</code></td>
          <td>mantém os intermédios <code>.s</code>/<code>.o</code> para debug</td>
        </tr>
      </tbody>
    </table>

    <div class="callout">
      <strong>Nota:</strong> o executável final é ligado com
      <code>gcc -no-pie</code>, que invoca o <code>ld</code> do sistema com
      crt e libc — necessário para <code>printf</code> usado por
      <code>print</code>.
    </div>
  </div>
</template>
