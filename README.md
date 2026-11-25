# Console Banking App

A menu-driven banking program in C. Accounts and transaction history are stored in a SQLite database (`bank.db`), so they are still there the next time you run it.

## Features

- Check balance, deposit, withdraw, view history, create account
- Every transaction is recorded with a timestamp
- Account IDs start at 1001
- Amounts must be from 1 to 10,000,000; a withdrawal cannot exceed the balance; a name cannot be empty
- A deposit or withdrawal updates the balance and writes its history record in one database transaction, so one cannot succeed without the other
- On the first run, three sample accounts are created

## Project layout

```
src/main.c        menu and input handling only
src/bank.c/.h     business rules (amount range, name checks)
src/db.c/.h       the only file that uses SQLite: schema, queries, transactions
src/status.h      result codes and callback types shared by all files
third_party/sqlite/   SQLite 3.54.1 amalgamation (sqlite3.c, sqlite3.h)
```

## Build

SQLite is large, so compile it once and reuse the result.

macOS / Linux / MinGW:

```
gcc -O2 -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -c third_party/sqlite/sqlite3.c -o sqlite3.o
gcc -std=c11 -Wall -Wextra -Wpedantic -Isrc -Ithird_party/sqlite src/main.c src/bank.c src/db.c sqlite3.o -o bank
```

Windows (Visual Studio Developer Command Prompt):

```
cl /O2 /DSQLITE_THREADSAFE=0 /DSQLITE_OMIT_LOAD_EXTENSION /Isrc /Ithird_party\sqlite src\main.c src\bank.c src\db.c third_party\sqlite\sqlite3.c /Fe:bank.exe
```

## Run

```
./bank          (Windows: bank.exe)
```

Run it from the project folder. `bank.db` is created in the current directory on the first run. Delete `bank.db` to start again with fresh sample data.