package tests.marshalling.classes;

import tests.marshalling.StdVector;
import utest.Assert;
import utest.Test;

abstract MyVector(StdVector<Int>) {
    public var size (get, never) : Int;

    function get_size() {
        return this.size();
    }

    public function new() {
        this = new StdVector<Int>(50);
    }

    public function resize(_size:Int) {
        this.resize(_size);
    }
}

class TestAbstractValueType extends Test {
    function test_cast_to_underlying() {
        final v = new MyVector();

        Assert.equals(50, (cast v : StdVector<Int>).size());
    }

    function test_property_access() {
        final v = new MyVector();
        
        Assert.equals(50, v.size);
    }

    function test_mutating_abstract() {
        final v = new MyVector();

        v.resize(100);

        Assert.equals(100, v.size);
        Assert.equals(100, (cast v : StdVector<Int>).size());
    }

    function test_casting_copy() {
        final v = new MyVector();
        final i = (cast v : StdVector<Int>);

        v.resize(100);

        Assert.equals(100, v.size);
        Assert.equals( 50, i.size());
    }
}