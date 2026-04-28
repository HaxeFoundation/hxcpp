#include <hxcpp.h>
#include <Managed.hpp>

int foo::bar::standard_naming_obj::constNumber = 300;

foo::bar::standard_naming_obj::standard_naming_obj() : number(0) {}
foo::bar::standard_naming_obj::standard_naming_obj(int inNumber) : number(inNumber) {}

int foo::bar::standard_naming_obj::multiply(int input)
{
    return number * input;
}

::String foo::bar::standard_naming_obj::toString()
{
    return ::String::create("My Custom Managed Type");
}

foo::bar::standard_naming_obj* foo::bar::standard_naming_obj::create(int inNumber)
{
    return new foo::bar::standard_naming_obj(inNumber);
}