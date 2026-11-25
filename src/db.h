#ifndef DB_H
#define DB_H

#include "status.h"
#include <stddef.h>

Status db_init(void);
void db_close(void);

Status db_create_account(const char *name, long long *new_id);
Status db_account_exists(long long id);
Status db_get_account(long long id, char *name_out, size_t name_size, long long *balance_out);
Status db_deposit(long long id, long long amount, long long *new_balance);
Status db_withdraw(long long id, long long amount, long long *new_balance);
Status db_history(long long id, HistoryRowFn fn, void *ctx);
Status db_list_accounts(AccountRowFn fn, void *ctx);

#endif