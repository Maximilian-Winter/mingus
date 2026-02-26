SQLite3 Amalgamation — Setup Instructions
==========================================

The SQLite3 example requires the SQLite3 amalgamation source files in this
directory. These are not checked into the repository due to their size.

Required files:
  extern_c/sqlite3.c    (~250K lines, amalgamation source)
  extern_c/sqlite3.h    (~13K lines, public API header)

Download:
  1. Visit https://sqlite.org/download.html
  2. Download "sqlite-amalgamation-XXXXXXX.zip" (Source Code section)
  3. Extract sqlite3.c and sqlite3.h into this directory (extern_c/)

Or via command line:
  curl -L https://sqlite.org/2025/sqlite-amalgamation-3480000.zip -o sqlite3.zip
  tar -xf sqlite3.zip
  copy sqlite-amalgamation-3480000\sqlite3.c extern_c\
  copy sqlite-amalgamation-3480000\sqlite3.h extern_c\

Regenerating Mingus bindings:
  python mingus_bind_gen.py examples/extern_c/sqlite3.h ^
      --prefix sqlite3_ --prefix SQLITE_ --module SQLite3 ^
      -o examples/SQLite3.mingus

Building the example:
  cd examples
  build_sqlite3_example.bat
