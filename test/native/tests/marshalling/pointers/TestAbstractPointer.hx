package tests.marshalling.pointers;

import utest.Assert;
import utest.Test;

abstract MyContext(Context) {
    public var number (get, never) : Int;

    function get_number() {
        return this.number;
    }
    
    public function new() {
        this = Context.create();
    }

    public function double() {
        this.number *= 2;
    }
}

class TestAbstractPointer extends Test {
    function test_cast_to_underlying() {
        final v = new MyContext();

        Assert.equals(7, (cast v : Context).number);
    }

    function test_property_access() {
        final v = new MyContext();
        
        Assert.equals(7, v.number);
    }

    function test_mutating_abstract() {
        final v = new MyContext();

        v.double();

        Assert.equals(14, v.number);
        Assert.equals(14, (cast v : Context).number);
    }
}
