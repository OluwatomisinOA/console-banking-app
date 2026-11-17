# Console Banking App in C

A lightweight command-line banking application written in standard C featuring persistent file storage, transaction history tracking, and input validation.

## Features

* **Account Management:** Create new bank accounts with auto-incrementing Account IDs starting from `1001`.
* **Core Transactions:** Perform deposits and withdrawals with balance, amount, and capacity validation.
* **Balance & History:** Check real-time account balances and view itemized transaction histories (`DEPOSIT` / `WITHDRAW`).
* **File Persistence:** Automatically saves all accounts and transaction logs to `accounts.txt` and reloads them on subsequent runs.
* **Automatic Seeding:** Generates sample dummy accounts on first launch if `accounts.txt` does not exist yet.

## Getting Started

### Prerequisites

* A C compiler such as `gcc` or `clang`

### Compilation & Execution

1. Clone the repository:
   ```bash
   git clone <your-repo-url>
   cd <your-repo-folder>
   ```

2. Compile the source code:
   ```bash
   gcc -Wall -Wextra -o banking_app main.c
   ```

3. Run the application:
   ```bash
   ./banking_app
   ```

## Usage

Upon launch, the application displays all active accounts and presents the main menu:

```text
--- Menu ---
1. Balance
2. Deposit
3. Withdraw
4. History
5. Create Account
6. Exit
```

All changes made during a session are automatically saved to `accounts.txt` after every transaction or account creation.

## Project Structure

* `main.c` — Core application logic, data structures (`Account`, `Transaction`), file I/O, and CLI menu loop.
* `accounts.txt` — Auto-generated flat-file database storing account details and transaction records.