#include <hxcpp.h>
#include <Managed.hpp>

int hx::fooextern::constNumber = 300;

hx::fooextern::fooextern() {}

int hx::fooextern::doubleNumber()
{
    return number * 2;
}

::String hx::fooextern::toString()
{
    return ::String::create("fooextern");
}

hx::fooextern* hx::fooextern::create(int number)
{
    auto ptr = new hx::fooextern();

    ptr->number = number;

    return ptr;
}

int hx::WithClosure::ReturnSeven()
{
    return 7;
}

int foo::bar::StandardLayoutExtern_obj::doubleNumber(int input)
{
    return input * 2;
}

foo::bar::StandardLayoutExtern foo::bar::StandardLayoutExtern_obj::create()
{
    return foo::bar::StandardLayoutExtern(new StandardLayoutExtern_obj());
}

namespace hx
{
    HX_DEFINE_DYNAMIC_FUNC0(WithClosure, ReturnSeven, return)
}