#pragma once

HX_DECLARE_CLASS2(foo,bar,standard_naming)

namespace foo
{
    namespace bar
    {
        struct standard_naming_obj : public ::hx::Object {
            static int constNumber;

            int number;

            standard_naming_obj();
            standard_naming_obj(int inNumber);

            int multiply(int input);

            ::String toString() override;

            static standard_naming_obj* create(int inNumber);
        };
    }

    struct WithClosure : public ::hx::Object {
        int ReturnSeven();

        ::Dynamic ReturnSeven_dyn();
    };
}