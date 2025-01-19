#pragma once

struct ctx {
    int number;

    ctx();

    int Double();
};

ctx* create();
ctx* create_null();