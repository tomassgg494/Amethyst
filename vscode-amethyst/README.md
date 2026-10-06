# Amethyst for VS Code

Syntax highlighting, snippets and one-click compile/run for the
[Amethyst](https://github.com/tomasdias/Amethyst) language (`.amt` files).

## Features

- **Syntax highlighting** for keywords, types, strings, numbers, comments,
  builtins (`len`, `push`, `pop`, `sqrt`, …) and struct names
- **Language configuration**: `//` line comments, bracket matching and
  automatic indenting on `{`
- **Snippets**: `main`, `fn`, `struct`, `impl`, `new`, `for`, `while`,
  `print`, `push`, `free`, and more (type the prefix and press Tab)
- **Compile Current File** and **Compile and Run Current File** — both run
  in an integrated terminal, in the file's own folder, so relative paths in
  the program behave as they do from a shell
- Buttons for both commands in the editor title bar when an `.amt` file is
  open

## Requirements

The Amethyst compiler, `amethystc`, must be reachable from the terminal.
Install the Dev Kit package (or build the compiler and put it on `PATH`):

```sh
make && sudo make deb
```

## Settings

| Setting | Default | Meaning |
| --- | --- | --- |
| `amethyst.compilerPath` | `amethystc` | Compiler to invoke (an absolute path works too) |
| `amethyst.outputDirectory` | *(system temp dir)* | Where the compiled binary is written |

## Install

**From source (development host)**

1. Open this folder in VS Code and press `F5` (Run > Start Debugging) to
   launch an Extension Development Host with the extension loaded.

**As a package**

```sh
cd vscode-amethyst
npm run package          # npx @vscode/vsce package --no-dependencies
code --install-extension amethyst-0.1.0.vsix
```

**By hand**

Copy this folder into your extensions directory as `amethyst-0.1.0`, for
example `~/.vscode/extensions/amethyst-0.1.0` on Linux, and restart VS Code.

## License

MIT — see [LICENSE](LICENSE).
