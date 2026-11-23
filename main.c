#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

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
#define BAD_INDEX -5
#define NAME_BUF_LEN 128
#define LOAD_OK 0
#define LOAD_NO_FILE 1
#define LOAD_BAD_FILE 2
#define TEMP_FILE_NAME "accounts.tmp"
#define BAD_FILE_NAME "accounts.bad"


typedef struct {
    char type[10];
    long amount;
    char timestamp[25];
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

// This function records one history line
int add_transaction(int index, const char *type, long amount) {
    // Checks if the index is valid
    if (index < 0 || index >= account_count) {
        return BAD_INDEX;
    }
    // Checks if the history is full
    if (accounts[index].txn_count >= MAX_TXNS) {
        return FULL_HISTORY;
    }

    int slot = accounts[index].txn_count;
    snprintf(accounts[index].history[slot].type, sizeof(accounts[index].history[slot].type), "%s", type);
    accounts[index].history[slot].amount = amount;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t != NULL) {
        strftime(accounts[index].history[slot].timestamp, sizeof(accounts[index].history[slot].timestamp), "%Y-%m-%d %H:%M:%S", t);
    } else {
        snprintf(accounts[index].history[slot].timestamp, sizeof(accounts[index].history[slot].timestamp), "Unknown");
    }

    accounts[index].txn_count++;
    return SUCCESS;
}

// This function makes a new account
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
        return BAD_INDEX;
    }
    // Check if the amount is valid
    if (amount <= 0 || amount > MAX_AMT) {
        return BAD_AMT;
    }
    
    int is_withdraw = (strcmp(type, "WITHDRAW") == 0);
    int is_deposit = (strcmp(type, "DEPOSIT") == 0);
    if (!is_withdraw && !is_deposit) return BAD_TYPE;
    if (is_withdraw && accounts[index].balance < amount) return INSUFF_FUNDS;

    int status = add_transaction(index, type, amount);
    if (status != SUCCESS) return status;

    if (is_withdraw) {
        accounts[index].balance -= amount;
    } else {
        accounts[index].balance += amount;
    }
    return SUCCESS;
}

// This function creates sample accounts
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
    FILE *fp = fopen(TEMP_FILE_NAME, "w");
    if (fp == NULL) {
        perror("fopen");
        return -1;
    }
    fprintf(fp, "%d\n", account_count);
    for (int i = 0; i < account_count; i++) {
        fprintf(fp, "%d|%s|%ld|%d\n", accounts[i].id, accounts[i].name, accounts[i].balance, accounts[i].txn_count);
        for (int j = 0; j < accounts[i].txn_count; j++) {
            fprintf(fp, "%s|%ld|%s\n", accounts[i].history[j].type, accounts[i].history[j].amount, accounts[i].history[j].timestamp);
        }
    }

    int write_failed = ferror(fp);
    if (fclose(fp) != 0) {
        write_failed = 1;
    }
    if (write_failed) {
        remove(TEMP_FILE_NAME);
        return -1;
    }
    if (rename(TEMP_FILE_NAME, DATA_FILE_NAME) != 0) {
        perror("rename");
        remove(TEMP_FILE_NAME);
        return -1;
    }
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

// This function sets the account counts to zero, next id to start id and clears the array
void reset_data(void) {
    account_count = 0;
    next_id = START_ID;
    memset(accounts, 0, sizeof(accounts));
}

// Returns 1 if account at array position i is internally consistent, else 0
int account_is_valid(int i) {
    const Account *a = &accounts[i];

    if (a->id < START_ID) {
        return 0;
    }
    for (int k = 0; k < i; k++) {              // compare against earlier accounts only
        if (accounts[k].id == a->id) {
            return 0;                          // duplicate ID
        }
    }

    long running = 0;
    for (int j = 0; j < a->txn_count; j++) {
        const Transaction *t = &a->history[j];
        if (t->amount <= 0 || t->amount > MAX_AMT) {
            return 0;
        }
        if (strcmp(t->type, "DEPOSIT") == 0) {
            running += t->amount;
        } else if (strcmp(t->type, "WITHDRAW") == 0) {
            if (running < t->amount) {
                return 0;                      
            }
            running -= t->amount;
        } else {
            return 0;                     
        }
    }
    return running == a->balance;            
}

int fail_load(FILE *fp) {
    fclose(fp);
    reset_data();
    return LOAD_BAD_FILE;
}
// This function loads the accounts from a file
int load_accounts(void) {
    FILE *fp = fopen(DATA_FILE_NAME, "r");
    if (fp == NULL) {
        return LOAD_NO_FILE;
    }

    int count;
    if ((fscanf(fp, "%d\n", &count) != 1) || count < 0 || count > MAX_ACCOUNTS) {
        return fail_load(fp);
    }

    for (int i = 0; i < count; i++) {
        int acc_line = fscanf(fp, "%d|%49[^|]|%ld|%d\n", &accounts[i].id,
                              accounts[i].name, &accounts[i].balance, &accounts[i].txn_count);
        if (acc_line != 4) {
            return fail_load(fp);
        }
        if (accounts[i].txn_count < 0 || accounts[i].txn_count > MAX_TXNS) {
            return fail_load(fp);
        }

        for (int j = 0; j < accounts[i].txn_count; j++) {
            int txn_line = fscanf(fp, "%9[^|]|%ld|%24[^\n]\n", accounts[i].history[j].type,
                                  &accounts[i].history[j].amount, accounts[i].history[j].timestamp);
            if (txn_line != 3) {
                return fail_load(fp);
            }
        }

        if (!account_is_valid(i)) {            
            return fail_load(fp);
        }
        if (accounts[i].id >= next_id) {
            next_id = accounts[i].id + 1;
        }
    }
    account_count = count;
    fclose(fp);
    return LOAD_OK;
}
// Helper to clear leftover characters like "\n" from the input buffer
void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void save_or_warn(void) {
    if (save_accounts() != 0) {
        printf("Warning: could not save changes\n");
    }
}

void print_menu(void) {
    printf("\n--- Menu ---\n");
    printf("1. Check Balance\n");
    printf("2. Deposit\n");
    printf("3. Withdraw\n");
    printf("4. View History\n");
    printf("5. Create Account\n");
    printf("6. Exit\n");
    printf("Choose an option: ");
}

void show_balance(int idx) {
    printf("Account: %s (ID: %d)\n", accounts[idx].name, accounts[idx].id);
    printf("Current Balance: %ld\n", accounts[idx].balance);
}

void show_history(int idx) {
    printf("\nTransaction History for %s (ID: %d):\n", accounts[idx].name, accounts[idx].id);
    if (accounts[idx].txn_count == 0) {
        printf("No transactions yet.\n");
    } else {
        for (int j = 0; j < accounts[idx].txn_count; j++) {
            printf("%d. [%-19s] %-8s | %ld\n", j + 1, accounts[idx].history[j].timestamp, accounts[idx].history[j].type, accounts[idx].history[j].amount);
        }
    }
}

void make_txn(int idx, const char *type) {
    long amount;
    printf("Enter amount to %s: ", type);
    if (scanf("%ld", &amount) != 1) {
        printf("Invalid amount.\n");
        clear_input_buffer();
        return;
    }
    clear_input_buffer();
    int status = apply_transaction(idx, type, amount);
    print_txn_status(status);
    if (status == SUCCESS) {
        save_or_warn();
        printf("New Balance: %ld\n", accounts[idx].balance);
    }
}

int ask_account(void) {
    int id;
    printf("Enter Account ID: ");
    if (scanf("%d", &id) != 1) {
        printf("Invalid Account ID.\n");
        clear_input_buffer();
        return -1;
    }
    clear_input_buffer();
    int got_acc_idx = find_account_index(id);
    if (got_acc_idx == -1) {printf("Error: Account ID %d not found.\n", id);}
    return got_acc_idx;
}

void do_create_account(void) {
    char name[NAME_BUF_LEN];
    printf("Enter account holder name: ");
    if (fgets(name, sizeof(name), stdin) != NULL) {
        if (strchr(name, '\n') == NULL) {
            clear_input_buffer();
        }
        name[strcspn(name, "\n")] = '\0';
        if (strlen(name) == 0) {
            printf("Name cannot be empty.\n");
            return;
        }
        if (strlen(name) >= MAX_NAME_LEN) {
            printf("Name too long!\n");
            return;   
        }
        if (strchr(name, '|') != NULL) {
            printf("Name cannot contain |\n");
            return;
        }
        int new_idx = create_account(name);
        if (new_idx == -1) {
            printf("Error: Maximum account limit reached.\n");
        } else {
            save_or_warn();
            printf("Account created! Your Account ID is: %d\n", accounts[new_idx].id);
        }
    }
}

void load_or_init(void) {
    // Load saved data or initialize dummy data if no file exists yet
    switch (load_accounts()) {
        case LOAD_OK:
            printf("Accounts loaded successfully.\n");
            break;
        case LOAD_NO_FILE:
            printf("No saved accounts found! Initializing with dummy data...\n");
            add_dummy_data();
            save_or_warn();
            break;
        case LOAD_BAD_FILE:
            printf("Error loading accounts! Initializing with dummy data...\n");
            remove(BAD_FILE_NAME);
            if (rename(DATA_FILE_NAME, BAD_FILE_NAME) != 0) {
                perror("rename");
            }
            add_dummy_data();
            save_or_warn();
            break;
    }
}

void print_accounts(void) {
    // Show loaded or dummy accounts immediately as stated in requirements
    printf("\n--- Active Accounts ---\n");
    for (int i = 0; i < account_count; i++) {
        printf("ID: %d | Name: %s | Balance: %ld\n", accounts[i].id, accounts[i].name, accounts[i].balance);
    }
}

// Main function to run the console banking app
int main(void) {
    printf("=== Welcome to the Console Banking App ===\n");
    load_or_init();
    print_accounts();

    int choice = 0;
    while (1) {
        print_menu();
        int result = scanf("%d", &choice);
        if (result == EOF) {
            save_or_warn();
            printf("\nGoodbye!\n");
            break;
        }
        if (result != 1) {
            printf("Invalid input. Please enter a number.\n");
            clear_input_buffer();
            continue;
        }

        clear_input_buffer(); // clears the leftover \n after reading the number

        if (choice == 6) { save_or_warn(); printf("Data saved. Goodbye!\n"); break; }
        if (choice == 5) { do_create_account(); continue; }
        if (choice < 1 || choice > 4) { printf("Invalid choice. Please select 1-6.\n"); continue; }

        int idx = ask_account();
        if (idx == -1) continue;

        switch (choice) {
            case 1: show_balance(idx); break;
            case 2: make_txn(idx, "DEPOSIT"); break;
            case 3: make_txn(idx, "WITHDRAW"); break;
            case 4: show_history(idx); break;
        }
    }
    return 0;
}