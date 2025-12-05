#pragma once

struct ctx {
    int number;

    ctx();

    int Double();

    static ctx* create();
    static ctx* create_null();
};

void ctx_ptr(ctx* pCtx);
void ctx_ptr_ptr(ctx** ppCtx);
void ctx_void_ptr(void* pCtx);
void ctx_void_ptr_ptr(void** ppCtx);