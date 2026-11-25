#include "db.h"
#include "sqlite3.h"
#include <stdio.h>

static sqlite3 *g_db = NULL;

static Status exec_sql(const char *sql) {
    char *errmsg = NULL;
    int rc = sqlite3_exec(g_db, sql, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        if (errmsg != NULL) {
            fprintf(stderr, "db error: %s\n", errmsg);
        }
        sqlite3_free(errmsg);
        return ERR_DB;
    }
    return OK;
}

void db_close(void) {
    if (g_db != NULL) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

static Status get_user_version(int *out) {
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, "PRAGMA user_version;", -1, &stmt, NULL);
    if (rc != SQLITE_OK) return ERR_DB;
    Status status = ERR_DB;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *out = sqlite3_column_int(stmt, 0);
        status = OK;
    }
    sqlite3_finalize(stmt);
    return status;
}

Status db_account_exists(long long id) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(g_db, "SELECT 1 FROM accounts WHERE id = ?1;", -1, &stmt, NULL) != SQLITE_OK) return ERR_DB;
    int rc = sqlite3_bind_int64(stmt, 1, id);
    if (rc == SQLITE_OK) {
        rc = sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    if (rc == SQLITE_ROW) {
        return OK;
    }
    if (rc == SQLITE_DONE) {
        return ERR_NOT_FOUND;
    }
    return ERR_DB;
}

static Status insert_account_in_tx(const char *name, long long *new_id) {
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, "INSERT INTO accounts (name) VALUES (?1);", -1, &stmt, NULL);
    if (rc != SQLITE_OK) return ERR_DB;

    rc = sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    if (rc == SQLITE_OK) {
        rc = sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return ERR_DB;

    *new_id = sqlite3_last_insert_rowid(g_db);
    return OK;
}

static Status apply_in_tx(long long id, long long amount, int is_withdraw) {
    const char *update_sql = is_withdraw
        ? "UPDATE accounts SET balance = balance - ?1 WHERE id = ?2 AND balance >= ?1;"
        : "UPDATE accounts SET balance = balance + ?1 WHERE id = ?2;";
    const char *type = is_withdraw ? "WITHDRAW" : "DEPOSIT";

    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, update_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return ERR_DB;

    rc = sqlite3_bind_int64(stmt, 1, amount);
    if (rc == SQLITE_OK) rc = sqlite3_bind_int64(stmt, 2, id);
    if (rc == SQLITE_OK) rc = sqlite3_step(stmt);
    int changed = sqlite3_changes(g_db);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return ERR_DB;
    if (changed == 0) {
        if (is_withdraw && db_account_exists(id) == OK) return ERR_FUNDS;
        return ERR_NOT_FOUND;
    }

    stmt = NULL;
    rc = sqlite3_prepare_v2(g_db,"INSERT INTO transactions (account_id, type, amount) VALUES (?1, ?2, ?3);",-1,&stmt,NULL);
    if (rc != SQLITE_OK) return ERR_DB;

    rc = sqlite3_bind_int64(stmt, 1, id);
    if (rc == SQLITE_OK) rc = sqlite3_bind_text(stmt, 2, type, -1, SQLITE_TRANSIENT);
    if (rc == SQLITE_OK) rc = sqlite3_bind_int64(stmt, 3, amount);
    if (rc == SQLITE_OK) rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return ERR_DB;
    return OK;
}

static Status seed_sample_data(void) {
    long long a = 0, b = 0, c = 0;

    if (insert_account_in_tx("James White", &a) != OK) return ERR_DB;
    if (apply_in_tx(a, 20000, 0) != OK) return ERR_DB;

    if (insert_account_in_tx("Max Altman", &b) != OK) return ERR_DB;
    if (apply_in_tx(b, 300000, 0) != OK) return ERR_DB;

    if (insert_account_in_tx("David Dennis", &c) != OK) return ERR_DB;
    if (apply_in_tx(c, 55000, 0) != OK) return ERR_DB;

    if (apply_in_tx(b, 50000, 1) != OK) return ERR_DB;

    return OK;
}


static Status init_new_database(void) {
    if (exec_sql("BEGIN IMMEDIATE;") != OK) return ERR_DB;
    if (exec_sql(
        "CREATE TABLE IF NOT EXISTS accounts ("
        "   id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "   name TEXT NOT NULL CHECK (length(trim(name)) > 0),"
        "   balance INTEGER NOT NULL DEFAULT 0 CHECK (balance >= 0)"
        ");"
    ) != OK) goto fail;
    if (exec_sql(
        "CREATE TABLE IF NOT EXISTS transactions ("
        "   id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "   account_id INTEGER NOT NULL REFERENCES accounts(id),"
        "   type TEXT NOT NULL CHECK (type IN ('DEPOSIT','WITHDRAW')),"
        "   amount INTEGER NOT NULL CHECK (amount > 0 AND amount <= 10000000),"
        "   timestamp TEXT NOT NULL DEFAULT (datetime('now','localtime'))"
        ");"
    ) != OK) goto fail;
    if (exec_sql("INSERT INTO sqlite_sequence (name, seq) VALUES ('accounts', 1000);") != OK) goto fail;
    if (seed_sample_data() != OK) goto fail;
    if (exec_sql("PRAGMA user_version = 1;") != OK) goto fail;
    if (exec_sql("COMMIT;") != OK) goto fail;
    return OK;
fail:
    exec_sql("ROLLBACK;");
    return ERR_DB;
}

Status db_init(void) {
    int rc = sqlite3_open("bank.db", &g_db);
    if (rc != SQLITE_OK) {
        db_close();
        return ERR_DB;
    }
    sqlite3_busy_timeout(g_db, 2000);
    if (exec_sql("PRAGMA foreign_keys = ON;") != OK) {
        db_close();
        return ERR_DB;
    }
    int version = 0;
    if (get_user_version(&version) != OK) {
        db_close();
        return  ERR_DB;
    }
    if (version == 0) {
        if (init_new_database() != OK) {
            db_close();
            return ERR_DB;
        }
    }
    return OK;
}

Status db_create_account(const char *name, long long *new_id) {
    return insert_account_in_tx(name, new_id);
}

Status db_get_account(long long id, char *name_out, size_t name_size, long long *balance_out) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(g_db, "SELECT name, balance FROM accounts WHERE id = ?1;", -1, &stmt, NULL) != SQLITE_OK) return ERR_DB;
    int rc = sqlite3_bind_int64(stmt, 1, id);
    if (rc == SQLITE_OK) rc = sqlite3_step(stmt);
    Status status = ERR_DB;
    if (rc == SQLITE_ROW) {
        if (name_out != NULL && name_size > 0) {
            snprintf(name_out, name_size, "%s", (const char *)sqlite3_column_text(stmt, 0));
        }
        if (balance_out != NULL) {
            *balance_out = sqlite3_column_int64(stmt, 1);
        }
        status = OK;
    } else if (rc == SQLITE_DONE) {
        status = ERR_NOT_FOUND;
    }
    sqlite3_finalize(stmt);
    return status;
}

Status db_list_accounts(AccountRowFn fn, void *ctx) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(g_db, "SELECT id, name, balance FROM accounts ORDER BY id;", -1, &stmt, NULL) != SQLITE_OK) return ERR_DB;
    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        fn(sqlite3_column_int64(stmt, 0),
           (const char *)sqlite3_column_text(stmt, 1),
           sqlite3_column_int64(stmt, 2),
           ctx);
    }
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? OK : ERR_DB;
}

Status db_history(long long id, HistoryRowFn fn, void *ctx) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(g_db, "SELECT timestamp, type, amount FROM transactions WHERE account_id = ?1 ORDER BY id;", -1, &stmt, NULL) != SQLITE_OK) return ERR_DB;
    int rc = sqlite3_bind_int64(stmt, 1, id);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return ERR_DB;
    }
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        fn((const char *)sqlite3_column_text(stmt, 0),
           (const char *)sqlite3_column_text(stmt, 1),
           sqlite3_column_int64(stmt, 2),
           ctx);
    }
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? OK : ERR_DB;
}

static Status run_money(long long id, long long amount, int is_withdraw, long long *new_balance) {
    if (exec_sql("BEGIN IMMEDIATE;") != OK) return ERR_DB;

    Status st = apply_in_tx(id, amount, is_withdraw);
    if (st != OK) goto rollback;

    st = db_get_account(id, NULL, 0, new_balance);
    if (st != OK) goto rollback;

    if (exec_sql("COMMIT;") != OK) {
        st = ERR_DB;
        goto rollback;
    }
    return OK;

rollback:
    exec_sql("ROLLBACK;");
    return st;
}

Status db_deposit(long long id, long long amount, long long *new_balance) {
    return run_money(id, amount, 0, new_balance);
}

Status db_withdraw(long long id, long long amount, long long *new_balance) {
    return run_money(id, amount, 1, new_balance);
}