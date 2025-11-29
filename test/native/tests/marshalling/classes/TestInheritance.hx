package tests.marshalling.classes;

import utest.Assert;
import utest.Test;

@:semantics(value)
@:include('Base.hpp')
@:cpp.ValueType
private extern class Base {
    function foo():Int;
}

@:semantics(value)
@:include('Child.hpp')
@:cpp.ValueType
private extern class Child extends Base {
    function new():Void;

    function bar():Int;
}

class TestInheritance extends Test {
    function test_copying_to_a_more_specific_type() {
        final o = create_child();
        final c : Child = cast o;

        Assert.equals(10, c.foo());
        Assert.equals(20, c.bar());
    }

    function test_copying_to_a_more_specific_type_promoted() {
        final o = create_child();
        final c : Child = cast o;
        final f = () -> {
            return c.bar();
        }

        f();

        Assert.equals(10, c.foo());
        Assert.equals(20, c.bar());
    }

    function test_casting_var_to_base() {
        final o = create_child();
        final v = (cast o : Child).bar();

        Assert.equals(20, v);
    }

    function test_passing_child_to_child_function() {
        final o = new Child();
        final v = pass_child(o);

        Assert.equals(10, v);
    }

    function test_passing_child_to_base_function() {
        final o = new Child();
        final v = pass_base(o);

        Assert.equals(7, v);
    }

    function test_reassigning() {
        var o : Base = new Child();

        Assert.equals(7, o.foo());

        o = new Child();

        Assert.equals(7, o.foo());
    }

    function create_child() : Base {
        return new Child();
    }

    function pass_child(c : Child) {
        return c.foo();
    }

    function pass_base(b : Base) {
        return b.foo();
    }
}