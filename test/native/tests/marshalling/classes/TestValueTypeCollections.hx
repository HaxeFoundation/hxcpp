package tests.marshalling.classes;

import utest.Assert;
import utest.Test;

class TestValueTypeCollections extends Test {
    public function test_push_copy_to_array() {
        final a = [];

        // prevent haxe optimising the array away
        a.resize(10);
        a.resize(0);

        a.push(new StdVector<Int>());
        
        Assert.equals(0, a[0].size());
    }

    public function test_mutate_array_element() {
        final a = [ new StdVector<Int>() ];
        final s = 100;

        // prevent haxe optimising the array away
        a.resize(10);
        a.resize(1);

        a[0].resize(s);

        Assert.equals(s, a[0].size());
    }

    public function test_copy_array_element() {
        final a = [ new StdVector<Int>() ];
        final s = 100;
        final c = a[0];

        c.resize(s);

        // prevent haxe optimising the array away
        a.resize(10);
        a.resize(1);

        Assert.equals(0, a[0].size());
        Assert.equals(s, c.size());
    }

    public function test_switch_on_array() {
        final a = [ new StdVector<Int>() ];
        final s = 100;

        // prevent haxe optimising the array away
        a.resize(10);
        a.resize(1);

        switch a {
            case [ elem ]:
                elem.resize(s);
                
                Assert.equals(s, a[0].size());
            default:
                Assert.fail('expected array to have one element');
        }
    }
}