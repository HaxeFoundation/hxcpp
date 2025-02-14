package tests.marshalling.classes;

import utest.Assert;
import utest.Test;

interface IBar {
    var v : StdVector<Int>;

    function get() : StdVector<Int>;

    function multiply(v : StdVector<Int>) : Int;
}

class Bar implements IBar {
    public var v : StdVector<Int>;

    public function new() {
        v = new StdVector<Int>(7);
    }

    public function get() {
        return v;
    }

    public function multiply(v:StdVector<Int>) {
        v.resize(v.size() * 2);

        return v.size();
    }
}

class TestInterfaceValueType extends Test {
    function test_interface_field_access() {
        final f : IBar = new Bar();

        Assert.equals(7, f.v.size());
    }

    function test_mutating_interface_field() {
        final f : IBar = new Bar();

        f.v.resize(100);

        Assert.equals(100, f.v.size());
    }

    function test_interface_field_copy() {
        final f : IBar = new Bar();
        final v = f.v;

        v.resize(100);

        Assert.equals(100, v.size());
        Assert.equals(7, f.v.size());
    }

    function test_interface_function_copy() {
        final f : IBar = new Bar();
        final v = f.get();

        v.resize(100);

        Assert.equals(100, v.size());
        Assert.equals(7, f.v.size());
    }

    function test_interface_field_assignment() {
        final f : IBar = new Bar();

        f.v = new StdVector(100);

        Assert.equals(100, f.v.size());
    }

    function test_interface_function_pass() {
        final f : IBar = new Bar();
        final v = new StdVector<Int>(7);
        final s = f.multiply(v);

        Assert.equals(7, v.size());
        Assert.equals(14, s);
    }

    function test_interface_function_call() {
        final f : IBar = new Bar();
        final s = f.get().size();

        Assert.equals(7, s);
    }

    function test_interface_reflect_variable() {
        final f : IBar = new Bar();
        final s = (Reflect.field(f, 'v') : StdVector<Int>).size();
        
        Assert.equals(7, s);
    }

    function test_interface_reflect_variable_mutation() {
        final f : IBar = new Bar();

        (Reflect.field(f, 'v') : StdVector<Int>).resize(100);
        
        Assert.equals(100, f.v.size());
    }

    function test_interface_reflect_variable_assignment() {
        final f : IBar = new Bar();

        Reflect.setField(f, 'v', new StdVector<Int>(100));

        Assert.equals(100, f.v.size());
    }
}