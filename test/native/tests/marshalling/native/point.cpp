#include <hxcpp.h>
#include <point.hpp>

hx::maths::point::point(double _x, double _y) : x(_x), y(_y) {}

void ::hx::maths::point::point_vec(std::vector<::hx::maths::point>& v) {
    v[0].x = 300;
}