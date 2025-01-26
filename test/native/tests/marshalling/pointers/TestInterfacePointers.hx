package tests.marshalling.pointers;

import utest.Assert;
import utest.Test;

interface IBar {
    var c : Context;

    function get() : Context;

    function assign(c : Context) : Void;
}

class Bar implements IBar {
    public var c : Context;

    public function new() {
        c = Context.create();
    }

    public function assign(c:Context) {
        c.number = 20;
    }

    public function get():Context {
        return c;
    }
}

class TestInterfacePointers extends Test {
    function test_interface_field_access() {
        final f : IBar = new Bar();

        Assert.equals(7, f.c.number);
    }

    function test_mutating_interface_field() {
        final f : IBar = new Bar();

        f.c.number = 20;

        Assert.equals(20, f.c.number);
    }

    function test_interface_field_copy() {
        final f : IBar = new Bar();
        final c = f.c;

        c.number = 20;

        Assert.isTrue(c == f.c);
        Assert.equals(20, c.number);
        Assert.equals(20, f.c.number);
    }

    function test_interface_field_assignment() {
        final f : IBar = new Bar();
        final old = f.c;

        f.c = Context.create();

        Assert.isTrue(old != f.c);
    }

    function test_interface_function_return() {
        final f : IBar = new Bar();
        final c = f.get();

        Assert.isTrue(f.c == c);
    }

    function test_interface_function_call() {
        final f : IBar = new Bar();
        final c = Context.create();

        f.assign(c);

        Assert.equals(20, c.number);
    }

    function test_interface_reflect_variable() {
        final f : IBar = new Bar();
        final s = (Reflect.field(f, 'c') : Context).number;
        
        Assert.equals(7, s);
    }

    function test_interface_reflect_variable_mutation() {
        final f : IBar = new Bar();

        (Reflect.field(f, 'c') : Context).number = 20;
        
        Assert.equals(20, f.c.number);
    }

    function test_interface_reflect_variable_assignment() {
        final f : IBar = new Bar();
        final old = f.c;

        Reflect.setField(f, 'c', Context.create());

        Assert.isTrue(old != f.c);
    }
}