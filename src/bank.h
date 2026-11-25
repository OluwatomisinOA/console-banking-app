#ifndef BANK_H
#define BANK_H

#include <stddef.h>
#include "status.h"

#define MAX_AMT 10000000
#define MAX_NAME_LEN 50

Status bank_init(void);
void bank_shutdown(void);

Status bank_create_account(const char *name, long long *new_id);
Status bank_account_exists(long long id);
Status bank_get_account(long long id, char *name_out, size_t name_size, long long *balance_out);
Status bank_deposit(long long id, long long amount, long long *new_balance);
Status bank_withdraw(long long id, long long amount, long long *new_balance);
Status bank_history(long long id, HistoryRowFn fn, void *ctx);
Status bank_list_accounts(AccountRowFn fn, void *ctx);

#endif