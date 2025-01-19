#pragma once

#include <Base.hpp>

struct Child : public Base {
    int foo() override;
    int bar();
};