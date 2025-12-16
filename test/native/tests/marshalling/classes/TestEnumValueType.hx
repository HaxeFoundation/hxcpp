package tests.marshalling.classes;

import haxe.EnumTools;
import utest.Assert;
import utest.Test;

enum FooEnum {
    Bar(v:StdVector<Int>);
}

class TestEnumValueType extends Test {
    function test_copy_into_enum() {
        final v = new StdVector<Int>(50);
        final e = FooEnum.Bar(v);

        switch e {
            case Bar(v_copy):
                v.resize(100);

                Assert.equals(100, v.size());
                Assert.equals( 50, v_copy.size());
        }
    }

    function test_mutating_enum() {
        final e = FooEnum.Bar(new StdVector<Int>(50));

        switch e {
            case Bar(v1):
                v1.resize(100);

                Assert.equals(100, v1.size());

                switch e {
                    case Bar(v2):
                        Assert.equals(100, v2.size());
                }
        }
    }

    function test_copy_from_enum() {
        final e = FooEnum.Bar(new StdVector<Int>(50));

        switch e {
            case Bar(v):
                final v_copy = v;

                v_copy.resize(100);

                Assert.equals(100, v_copy.size());
                Assert.equals( 50, v.size());
        }
    }

    function test_create_by_index() {
        final v1 = new StdVector<Int>(50);
        final e  = EnumTools.createByIndex(FooEnum, 0, [ v1 ]);

        switch e {
            case Bar(v2):
                v1.resize(100);

                Assert.equals(100, v1.size());
                Assert.equals( 50, v2.size());
        }
    }

    function test_create_by_name() {
        final v1 = new StdVector<Int>(50);
        final e  = EnumTools.createByName(FooEnum, 'Bar', [ v1 ]);

        switch e {
            case Bar(v2):
                v1.resize(100);

                Assert.equals(100, v1.size());
                Assert.equals( 50, v2.size());
        }
    }
}