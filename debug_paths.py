import sys
from pathlib import Path

# Simulate what parse() passes to _extract_macros
filepath = str(Path("tests/test_defines.h").resolve())
print(f"filepath from parse(): {filepath!r}")

target_norm = filepath.replace("\\", "/")
if sys.platform == "win32":
    target_norm = target_norm.lower()
print(f"target_norm: {target_norm!r}")

# Simulate what clang outputs
clang_line = 'H:/language_dev/mingus/tests/test_defines.h'
current = clang_line.replace("\\", "/")
if sys.platform == "win32":
    current = current.lower()
print(f"clang current: {current!r}")
print(f"Match: {current == target_norm}")

# Also check: what does clang -dD -E args look like from parse?
print(f"\ncompile_args would include: {filepath}")
