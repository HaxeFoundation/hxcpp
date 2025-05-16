package tests.marshalling.pointers;

import haxe.EnumTools;
import utest.Assert;
import utest.Test;

enum FooEnum {
    Bar(c:Context);
}

class TestEnumPointers extends Test {
    function test_into_enum() {
        final c = Context.create();
        final e = FooEnum.Bar(c);

        c.number = 20;

        switch e {
            case Bar(v):

                Assert.equals(20, c.number);
                Assert.equals(20, v.number);
        }
    }

    function test_mutating_enum() {
        final c = Context.create();
        final e = FooEnum.Bar(c);

        switch e {
            case Bar(v1):
                v1.number = 20;

                Assert.equals(20, v1.number);
                Assert.equals(20, c.number);

                switch e {
                    case Bar(v2):
                        Assert.equals(20, v2.number);
                }
        }
    }

    function test_copy_from_enum() {
        final e = FooEnum.Bar(Context.create());

        switch e {
            case Bar(v):
                final v_copy = v;

                v_copy.number = 20;

                Assert.equals(20, v_copy.number);
                Assert.equals(20, v.number);
        }
    }

    function test_create_by_index() {
        final c = Context.create();
        final e = EnumTools.createByIndex(FooEnum, 0, [ c ]);

        switch e {
            case Bar(v):
                Assert.equals(7, v.number);
        }
    }

    function test_create_by_name() {
        final c = Context.create();
        final e = EnumTools.createByName(FooEnum, 'Bar', [ c ]);

        switch e {
            case Bar(v):

                Assert.equals(7, v.number);
        }
    }
}