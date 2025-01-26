package tests.marshalling.pointers;

import utest.Assert;
import utest.Test;

class TestPointerCollections extends Test {
    function test_pushing_to_array() {
        final a = [];

        a.push(Context.create());

        Assert.equals(7, a[0].number);
    }

    function test_mutate_element() {
        final c = Context.create();
        final a = [ c ];

        a[0].number = 20;

        Assert.equals(20, c.number);
        Assert.equals(20, a[0].number);
    }

    function test_null_default() {
        final a = new Array<Context>();

        a.resize(1);

        Assert.isTrue(a[0] == null);
    }

    function test_setting_array_element() {
        final c = Context.create();
        final a = [ null ];

        a[0] = c;

        if (Assert.isTrue(a[0] != null)) {
            Assert.equals(7, a[0].number);
        }
    }

    function test_switch_on_array() {
        final c = Context.create();
        final a = [ c ];

        // prevent haxe optimising the array away
        a.resize(10);
        a.resize(1);

        switch a {
            case [ elem ]:
                elem.number = 20;

                Assert.equals(20, c.number);
                Assert.equals(20, a[0].number);
            default:
                Assert.fail('expected array to have one element');
        }
    }
}