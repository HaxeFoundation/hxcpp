#pragma once

#include <point.hpp>

class holder {
    static ::hx::maths::point p_static;

public:
    ::hx::maths::point p1;
    ::hx::maths::point p2;

    ::hx::maths::point* pPtr;

    holder() = default;
    holder(const ::hx::maths::point& _p1, const ::hx::maths::point& _p2);

    ::hx::maths::point create();
    ::hx::maths::point* get_static();
};