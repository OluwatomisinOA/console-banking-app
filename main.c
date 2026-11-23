#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_NAME_LEN 50
#define MAX_TXNS 100
#define MAX_ACCOUNTS 100
#define DATA_FILE_NAME "accounts.txt"
#define MAX_AMT 10000000
#define START_ID 1001
#define SUCCESS 0
#define BAD_AMT -1
#define INSUFF_FUNDS -2
#define FULL_HISTORY -3
#define BAD_TYPE -4

typedef struct {
    char type[10];
    long amount;
} Transaction;

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    long balance;
    Transaction history[MAX_TXNS];
    int txn_count;
} Account;

Account accounts[MAX_ACCOUNTS];
int account_count = 0;
int next_id = START_ID;

// This function record one history line
int add_transaction(int index, const char *type, long amount) {
    // Checks if the index is valid
    if (index < 0 || !(index < account_count)) {
        return BAD_TYPE;
    }
    // Checks if the history is full
    if (accounts[index].txn_count >= MAX_TXNS) {
        return FULL_HISTORY;
    }

    int slot = accounts[index].txn_count;
    snprintf(accounts[index].history[slot].type, sizeof(accounts[index].history[slot].type), "%s", type);
    accounts[index].history[slot].amount = amount;
    accounts[index].txn_count++;
    return SUCCESS;
}

// This function make a new account
int create_account(const char *name) {
    // Checks if the accounts array is full
    if (account_count >= MAX_ACCOUNTS) {
        return -1;
    }

    int idx = account_count;
    accounts[idx].id = next_id;
    snprintf(accounts[idx].name, sizeof(accounts[idx].name), "%s", name);
    accounts[idx].balance = 0;
    accounts[idx].txn_count = 0;
    next_id++;
    account_count++;
    return idx;
}

// The rule keeper
int apply_transaction(int index, const char *type, long amount) {
    // Check if the index is valid
    if (index < 0 || index >= account_count) {
        return -1;
    }
    // Check if the amount is valid
    if (amount <= 0 || amount > MAX_AMT) {
        return BAD_AMT;
    }
    // Checks if the transaction type is valid
    if ((strcmp(type, "WITHDRAW") != 0) && (strcmp(type, "DEPOSIT") != 0)) {
        return BAD_TYPE;
    }
    // Checks if there is room in the history
    if (accounts[index].txn_count >= MAX_TXNS) {
        return FULL_HISTORY;
    }

    // Handle withdrawal vs deposit cleanly
    if (strcmp(type, "WITHDRAW") == 0) {
        if (accounts[index].balance < amount) {
            return INSUFF_FUNDS;
        }
        accounts[index].balance -= amount;
    } else {
        accounts[index].balance += amount;
    }

    add_transaction(index, type, amount);
    return SUCCESS;
}

// This function gives the sample accounts
void add_dummy_data(void) {
    int acc1 = create_account("James White");
    apply_transaction(acc1, "DEPOSIT", 20000);
    int acc2 = create_account("Max Altman");
    apply_transaction(acc2, "DEPOSIT", 300000);
    int acc3 = create_account("David Dennis");
    apply_transaction(acc3, "DEPOSIT", 55000);
    apply_transaction(acc2, "WITHDRAW", 50000);
}

// Translates a user-facing account id into an array index 
int find_account_index(int id) {
    for (int i = 0; i < account_count; i++) {
        if (accounts[i].id == id) {
            return i; 
        }
    }
    return -1;
}

// This function saves the accounts to a file
int save_accounts(void) {
    FILE *fp = fopen(DATA_FILE_NAME, "w");
    if (fp == NULL) {
        perror("fopen");
        return -1;
    }
    fprintf(fp, "%d\n", account_count);
    for (int i = 0; i < account_count; i++) {
        fprintf(fp, "%d|%s|%ld|%d\n", accounts[i].id, accounts[i].name, accounts[i].balance, accounts[i].txn_count);
        for (int j = 0; j < accounts[i].txn_count; j++) {
            fprintf(fp, "%s %ld\n", accounts[i].history[j].type, accounts[i].history[j].amount);
        }
    }
    fclose(fp);
    return 0;
}

// Helper to print user-friendly messages for transaction status codes
void print_txn_status(int status) {
    if (status == SUCCESS) {
        printf("Transaction successful!\n");
    } else if (status == BAD_AMT) {
        printf("Error: Invalid amount. Must be between 1 and %d.\n", MAX_AMT);
    } else if (status == INSUFF_FUNDS) {
        printf("Error: Insufficient funds.\n");
    } else if (status == FULL_HISTORY) {
        printf("Error: Transaction history is full for this account.\n");
    } else {
        printf("Error: Transaction failed.\n");
    }
}

// This function loads the accounts from a file
int load_accounts(void) {
    FILE *fp = fopen(DATA_FILE_NAME, "r");
    if (fp == NULL) {
        return -1;
    }

    if (fscanf(fp, "%d\n", &account_count) != 1) {
        fclose(fp);
        return -1;
    }

    for (int i = 0; i < account_count; i++) {
        fscanf(fp, "%d|%49[^|]|%ld|%d\n", &accounts[i].id, accounts[i].name, &accounts[i].balance, &accounts[i].txn_count);
        
        // Update next_id AFTER reading the account ID from the file
        if (accounts[i].id >= next_id) {
            next_id = accounts[i].id + 1;
        }

        for (int j = 0; j < accounts[i].txn_count; j++) {
            fscanf(fp, "%9s %ld\n", accounts[i].history[j].type, &accounts[i].history[j].amount);
        }
    }

    fclose(fp);
    return 0;
}

// Helper to clear leftover characters like "\n" from the input buffer
void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Main function to run the console banking app
int main(void) {
    printf("=== Welcome to the Console Banking App ===\n");

    // Load saved data or initialize dummy data if no file exists yet
    if (load_accounts() != 0) {
        add_dummy_data();
        save_accounts();
    }

    // Show loaded or dummy accounts immediately as stated in requirements
    printf("\n--- Active Accounts ---\n");
    for (int i = 0; i < account_count; i++) {
        printf("ID: %d | Name: %s | Balance: %ld\n", accounts[i].id, accounts[i].name, accounts[i].balance);
    }

    int choice = 0;
    while (1) {
        printf("\n--- Menu ---\n");
        printf("1. Check Balance\n");
        printf("2. Deposit\n");
        printf("3. Withdraw\n");
        printf("4. View History\n");
        printf("5. Create Account\n");
        printf("6. Exit\n");
        printf("Choose an option: \n");

        int result = scanf("%d", &choice);
        if (result == EOF) {
            save_accounts();
            printf("Data saved. Goodbye!\n");
            break;
        }

        if (result != 1) {
            printf("Invalid input. Please enter a number.\n");
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();

        if (choice == 6) {
            save_accounts();
            printf("Data saved. Goodbye!\n");
            break;
        }

        if (choice == 5) {
            char name[MAX_NAME_LEN];
            printf("Enter account holder name: ");
            if (fgets(name, sizeof(name), stdin) != NULL) {
                name[strcspn(name, "\n")] = '\0';
                if (strlen(name) == 0) {
                    printf("Error: Name cannot be empty.\n");
                    continue;
                }

                int new_idx = create_account(name);
                if (new_idx == -1) {
                    printf("Error: Maximum account limit reached.\n");
                } else {
                    save_accounts();
                    printf("Account created! Your Account ID is: %d\n", accounts[new_idx].id);
                }
            }
            continue;
        }

        // Options 1 to 4 all require an account id first
        if (choice >= 1 && choice <= 4) {
            int id;
            printf("Enter Account ID: ");
            if (scanf("%d", &id) != 1) {
                printf("Invalid Account ID.\n");
                clear_input_buffer();
                continue;
            }
            clear_input_buffer();

            int idx = find_account_index(id);
            if (idx == -1) {
                printf("Error: Account ID %d not found.\n", id);
                continue;
            }

            if (choice == 1) {
                // Check balance
                printf("Account: %s (ID: %d)\n", accounts[idx].name, accounts[idx].id);
                printf("Current Balance: %ld\n", accounts[idx].balance);

            } else if (choice == 2) {
                // Deposit
                long amount;
                printf("Enter amount to deposit: ");
                if (scanf("%ld", &amount) != 1) {
                    printf("Invalid amount.\n");
                    clear_input_buffer();
                    continue;
                }
                clear_input_buffer();

                int status = apply_transaction(idx, "DEPOSIT", amount);
                print_txn_status(status);
                if (status == SUCCESS) {
                    save_accounts();
                    printf("New Balance: %ld\n", accounts[idx].balance);
                }

            } else if (choice == 3) {
                // Withdraw
                long amount;
                printf("Enter amount to withdraw: ");
                if (scanf("%ld", &amount) != 1) {
                    printf("Invalid amount.\n");
                    clear_input_buffer();
                    continue;
                }
                clear_input_buffer();

                int status = apply_transaction(idx, "WITHDRAW", amount);
                print_txn_status(status);
                if (status == SUCCESS) {
                    save_accounts();
                    printf("New Balance: %ld\n", accounts[idx].balance);
                }

            } else if (choice == 4) {
                // View history
                printf("\nTransaction History for %s (ID: %d):\n", accounts[idx].name, accounts[idx].id);
                if (accounts[idx].txn_count == 0) {
                    printf("No transactions yet.\n");
                } else {
                    for (int j = 0; j < accounts[idx].txn_count; j++) {
                        printf("%d. %-8s | %ld\n", j + 1, accounts[idx].history[j].type, accounts[idx].history[j].amount);
                    }
                }
            }
        } else {
            printf("Invalid choice. Please select 1-6.\n");
        }
    }

    return 0;
}