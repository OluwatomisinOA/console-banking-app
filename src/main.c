#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "bank.h"

#define NAME_BUF_LEN 128

// Clears leftover characters up to and including '\n' from the input buffer.
static void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Reads one whole line and requires the entire line to be a valid whole number.
static int read_ll(const char *prompt, long long *out) {
    char buf[64];
    printf("%s", prompt);
    if (fgets(buf, sizeof(buf), stdin) == NULL) return -1;

    if (strchr(buf, '\n') == NULL && !feof(stdin)) {
        clear_input_buffer();
        return 0;
    }

    char *end;
    errno = 0;
    long long v = strtoll(buf, &end, 10);
    if (end == buf || errno == ERANGE) return 0;     
    while (*end == ' ' || *end == '\t') end++;     
    if (*end != '\n' && *end != '\0') return 0;       
    *out = v;
    return 1;
}

// Turns a failure code into a user-facing message.
static void print_error(Status s) {
    switch (s) {
        case ERR_AMOUNT:
            printf("Error: Invalid amount. Must be between 1 and %d.\n", MAX_AMT);
            break;
        case ERR_FUNDS:
            printf("Error: Insufficient funds.\n");
            break;
        case ERR_NOT_FOUND:
            printf("Error: Account not found.\n");
            break;
        case ERR_NAME_EMPTY:
            printf("Name cannot be empty.\n");
            break;
        case ERR_NAME_LONG:
            printf("Name too long!\n");
            break;
        case ERR_DB:
            printf("Error: Database problem. Nothing was changed.\n");
            break;
        default:
            printf("Error: Something went wrong.\n");
            break;
    }
}

// Callback: bank layer calls this once per account row.
static void print_account_row(long long id, const char *name, long long balance, void *ctx) {
    (void)ctx;  
    printf("ID: %lld | Name: %s | Balance: %lld\n", id, name, balance);
}

// Callback: bank layer calls this once per history row, ctx points at a row counter.
static void print_history_row(const char *timestamp, const char *type, long long amount, void *ctx) {
    int *count = (int *)ctx;
    (*count)++;
    printf("%d. [%-19s] %-8s | %lld\n", *count, timestamp, type, amount);
}

static void print_menu(void) {
    printf("\n--- Menu ---\n");
    printf("1. Check Balance\n");
    printf("2. Deposit\n");
    printf("3. Withdraw\n");
    printf("4. View History\n");
    printf("5. Create Account\n");
    printf("6. Exit\n");
}

static void print_accounts(void) {
    printf("\n--- Active Accounts ---\n");
    Status s = bank_list_accounts(print_account_row, NULL);
    if (s != OK) print_error(s);
}

static void show_balance(long long id) {
    char name[MAX_NAME_LEN];
    long long balance;
    Status s = bank_get_account(id, name, sizeof(name), &balance);
    if (s != OK) {
        print_error(s);
        return;
    }
    printf("Account: %s (ID: %lld)\n", name, id);
    printf("Current Balance: %lld\n", balance);
}

static void show_history(long long id) {
    char name[MAX_NAME_LEN];
    Status s = bank_get_account(id, name, sizeof(name), NULL);
    if (s != OK) {
        print_error(s);
        return;
    }
    printf("\nTransaction History for %s (ID: %lld):\n", name, id);
    int count = 0;
    s = bank_history(id, print_history_row, &count);
    if (s != OK) {
        print_error(s);
    } else if (count == 0) {
        printf("No transactions yet.\n");
    }
}

static void make_txn(long long id, int is_withdraw) {
    const char *label = is_withdraw ? "WITHDRAW" : "DEPOSIT";
    char prompt[40];
    long long amount;
    long long new_balance = 0;

    snprintf(prompt, sizeof(prompt), "Enter amount to %s: ", label);
    if (read_ll(prompt, &amount) != 1) {
        printf("Invalid amount.\n");
        return;
    }
    Status s = is_withdraw ? bank_withdraw(id, amount, &new_balance)
                           : bank_deposit(id, amount, &new_balance);
    if (s != OK) {
        print_error(s);
        return;
    }
    printf("Transaction successful!\n");
    printf("New Balance: %lld\n", new_balance);
}

// Asks for an account ID and checks it exists.
static int ask_account(long long *id) {
    if (read_ll("Enter Account ID: ", id) != 1) {
        printf("Invalid Account ID.\n");
        return 0;
    }
    Status s = bank_account_exists(*id);
    if (s == ERR_NOT_FOUND) {
        printf("Error: Account ID %lld not found.\n", *id);
        return 0;
    }
    if (s != OK) {
        print_error(s);
        return 0;
    }
    return 1;
}

static void do_create_account(void) {
    char name[NAME_BUF_LEN];
    long long new_id = 0;

    printf("Enter account holder name: ");
    if (fgets(name, sizeof(name), stdin) == NULL) return;
    if (strchr(name, '\n') == NULL) {
        clear_input_buffer();
    }
    name[strcspn(name, "\n")] = '\0';

    Status s = bank_create_account(name, &new_id);
    if (s != OK) {
        print_error(s);
        return;
    }
    printf("Account created! Your Account ID is: %lld\n", new_id);
}

int main(void) {
    printf("=== Welcome to the Console Banking App ===\n");
    if (bank_init() != OK) {
        printf("Error: could not open the database.\n");
        return 1;
    }
    print_accounts();

    while (1) {
        print_menu();

        long long choice;
        int r = read_ll("Choose an option: ", &choice);
        if (r == -1) {
            bank_shutdown();
            printf("\nGoodbye!\n");
            break;
        }
        if (r == 0) {
            printf("Invalid input. Please enter a number.\n");
            continue;
        }

        if (choice == 6) { bank_shutdown(); printf("Goodbye!\n"); break; }
        if (choice == 5) { do_create_account(); continue; }
        if (choice < 1 || choice > 4) { printf("Invalid choice. Please select 1-6.\n"); continue; }

        long long id;
        if (!ask_account(&id)) continue;

        switch (choice) {
            case 1: show_balance(id); break;
            case 2: make_txn(id, 0); break;
            case 3: make_txn(id, 1); break;
            case 4: show_history(id); break;
        }
    }
    return 0;
}