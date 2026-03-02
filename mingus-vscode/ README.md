# Mingus Language Support

VSCode Syntax highlighting and language support for the Mingus programming language.

## Features

- Syntax highlighting for all Mingus constructs
- Auto-closing brackets, quotes, and block comments
- Smart indentation
- Comment toggling (Ctrl+/ for line, Shift+Alt+A for block)
- Code folding

## About Mingus

Mingus is a statically typed systems programming language that compiles to LLVM.
Named after Charles Mingus — fierce, precise, uncompromising.

[GitHub Repository](https://github.com/Maximilian-Winter/mingus)

## VSCode Installation
**To Install:**

```bash
npm install -g @vscode/vsce
vsce package
code --install-extension mingus-0.1.0.vsix
```