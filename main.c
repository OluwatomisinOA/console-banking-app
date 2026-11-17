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

// This function shows all the accounts
void print_all_accounts(void) {
    for (int i = 0; i < account_count; i++) {
        printf("Id: %d\nName: %s\nBalance: %ld\n", accounts[i].id, accounts[i].name, accounts[i].balance);
        printf("Transactions:\n");
        for (int j = 0; j < accounts[i].txn_count; j++) {
            printf("Type: %s | Amount: %ld\n", accounts[i].history[j].type, accounts[i].history[j].amount);
        }
    }
}

// This is the main function that runs the program
int main(void) {
    printf("Welcome to the Console Banking App\n");
    add_dummy_data();
    print_all_accounts();
    return 0;
}