#pragma once

HX_DECLARE_CLASS2(foo,bar,StandardLayoutExtern)

namespace foo
{
    namespace bar
    {
        struct StandardLayoutExtern_obj : public ::hx::Object {
            virtual int doubleNumber(int input);

            static StandardLayoutExtern create();
        };
    }
}

namespace hx
{
    struct fooextern : public ::hx::Object {
        static int constNumber;

        int number;

        fooextern();

        int doubleNumber();

        ::String toString() override;

        static fooextern* create(int number);
    };

    struct WithClosure : public ::hx::Object {
        int ReturnSeven();

        ::Dynamic ReturnSeven_dyn();
    };
}