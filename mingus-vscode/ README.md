# Mingus Language Support

Syntax highlighting and language support for the Mingus programming language.

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


# Installation
**To Install:**

**Option 1: Test in Dev Host**
```bash
# Open mingus-vscode folder in VSCode
# Press F5 to launch Extension Development Host
```

**Option 2: Install Locally**
```bash
# From mingus-vscode directory
code --install-extension .
# Or copy to: ~/.vscode/extensions/mingus-0.1.0/
```

**Option 3: Package for Distribution**
```bash
npm install -g @vscode/vsce
vsce package
# Creates mingus-0.1.0.vsix
```
