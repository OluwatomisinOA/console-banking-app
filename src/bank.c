#include <string.h>
#include "bank.h"
#include "db.h"

Status bank_init(void) {
    return db_init();
}

void bank_shutdown(void) {
    db_close();
}

Status bank_create_account(const char *name, long long *new_id) {
    if (name == NULL || name[strspn(name, " \t")] == '\0') {
        return ERR_NAME_EMPTY;
    }
    if (strlen(name) >= MAX_NAME_LEN) {
        return ERR_NAME_LONG;
    }
    return db_create_account(name, new_id);
}

Status bank_account_exists(long long id) {
    return db_account_exists(id);
}

Status bank_get_account(long long id, char *name_out, size_t name_size, long long *balance_out) {
    return db_get_account(id, name_out, name_size, balance_out);
}

Status bank_deposit(long long id, long long amount, long long *new_balance) {
    if (amount < 1 || amount > MAX_AMT) {
        return ERR_AMOUNT;
    }
    return db_deposit(id, amount, new_balance);
}

Status bank_withdraw(long long id, long long amount, long long *new_balance) {
    if (amount < 1 || amount > MAX_AMT) {
        return ERR_AMOUNT;
    }
    return db_withdraw(id, amount, new_balance);
}

Status bank_history(long long id, HistoryRowFn fn, void *ctx) {
    return db_history(id, fn, ctx);
}

Status bank_list_accounts(AccountRowFn fn, void *ctx) {
    return db_list_accounts(fn, ctx);
}