#include <hxcpp.h>
#include <holder.hpp>

::hx::maths::point holder::p_static;

holder::holder(const ::hx::maths::point& _p1, const ::hx::maths::point& _p2) : p1(_p1), p2(_p2), pPtr(new ::hx::maths::point(45, 67)) {}

::hx::maths::point holder::create()
{
    return ::hx::maths::point(p1.x + p2.x, p1.y + p2.y);
}

::hx::maths::point* holder::get_static() {
    return &p_static;
}