#ifndef MARSHALLING_NATIVE_CHILD_HPP
#define MARSHALLING_NATIVE_CHILD_HPP

#include <Base.hpp>

struct Child : public Base {
    int foo() override;
    int bar();
};

#endif
