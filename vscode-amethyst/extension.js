// Amethyst extension: compile and run the current .amt file in a terminal.
// Everything else (highlighting, snippets, comments) is declarative and
// lives in package.json and the files it points at.
"use strict";

const vscode = require("vscode");
const path = require("path");
const os = require("os");

function config() {
  return vscode.workspace.getConfiguration("amethyst");
}

function compilerPath() {
  const value = config().get("compilerPath");
  return value && value.trim() ? value.trim() : "amethystc";
}

// Quote a path so it survives a shell round trip (spaces, $, quotes).
function quote(value) {
  return '"' + String(value).replace(/(["\\$`])/g, "\\$1") + '"';
}

function outputFor(document) {
  const dir = config().get("outputDirectory");
  const base = path.basename(document.fileName, ".amt");
  return path.join(dir && dir.trim() ? dir.trim() : os.tmpdir(), base);
}

// The editor for the active .amt file, or null with a message explaining why.
function currentAmethystFile() {
  const editor = vscode.window.activeTextEditor;
  if (!editor || !editor.document) {
    vscode.window.showInformationMessage(
      "Open an Amethyst (.amt) file first."
    );
    return null;
  }
  const document = editor.document;
  if (document.languageId !== "amethyst") {
    vscode.window.showInformationMessage(
      "The active file is not an Amethyst (.amt) file."
    );
    return null;
  }
  if (document.isUntitled) {
    vscode.window.showWarningMessage(
      "Save the file before compiling it."
    );
    return null;
  }
  return document;
}

function runInTerminal(lines) {
  const terminal = vscode.window.createTerminal({ name: "Amethyst" });
  for (const line of lines) terminal.sendText(line, true);
  terminal.show();
}

async function compile(runAfter) {
  const document = currentAmethystFile();
  if (!document) return;
  if (document.isDirty) await document.save();

  const compiler = quote(compilerPath());
  const source = quote(document.fileName);
  const output = quote(outputFor(document));
  const compileLine = compiler + " -o " + output + " " + source;

  runInTerminal(
    runAfter
      ? [compileLine + " && " + output]
      : [compileLine]
  );
}

function activate(context) {
  context.subscriptions.push(
    vscode.commands.registerCommand("amethyst.compile", () => compile(false)),
    vscode.commands.registerCommand("amethyst.run", () => compile(true))
  );
}

function deactivate() {}

module.exports = { activate, deactivate };
