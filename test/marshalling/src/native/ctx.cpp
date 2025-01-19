#include <ctx.hpp>

ctx::ctx() : number(7) {}

int ctx::Double() {
    return number * 2;
}

ctx* create() {
    return new ctx();
}

ctx* create_null() {
    return nullptr;
}