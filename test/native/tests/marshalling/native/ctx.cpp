#include <ctx.hpp>

ctx::ctx() : number(7) {}

int ctx::Double() {
    return number * 2;
}

ctx* ctx::create() {
    return new ctx();
}

ctx* ctx::create_null() {
    return nullptr;
}

void ctx_ptr(ctx* pCtx) {
    pCtx->number = 20;
}

void ctx_ptr_ptr(ctx** ppCtx) {
    auto replacement = new ctx();

    replacement->number = 20;

    *ppCtx = replacement;
}

void ctx_void_ptr(void* pCtx) {
    auto casted = static_cast<ctx*>(pCtx);

    casted->number = 20;
}

void ctx_void_ptr_ptr(void** ppCtx) {
    auto replacement = new ctx();

    replacement->number = 20;

    *ppCtx = replacement;
}