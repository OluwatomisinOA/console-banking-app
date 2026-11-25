#ifndef STATUS_H
#define STATUS_H

typedef enum {
    OK = 0,
    ERR_AMOUNT,
    ERR_FUNDS,
    ERR_NOT_FOUND,
    ERR_NAME_EMPTY,
    ERR_NAME_LONG,
    ERR_DB
} Status;

typedef void (*HistoryRowFn) (
    const char *timestamp, 
    const char *type, 
    long long amount, 
    void *ctx
);

typedef void (*AccountRowFn) (
    long long id,
    const char *name,
    long long balance,
    void *ctx
);

#endif