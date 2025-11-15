package tests.marshalling.pointers;

import utest.Assert;
import utest.Test;

@:semantics(value)
@:include('Base.hpp')
@:cpp.PointerType
private extern class Base {
    function foo():Int;
}

@:semantics(value)
@:include('Child.hpp')
@:cpp.PointerType
private extern class Child extends Base {
    function bar():Int;
}

class TestInheritancePointers extends Test {
    function test_assigning_to_a_more_specific_type() {
        final o = create_child();
        final c : Child = cast o;

        Assert.equals(10, c.foo());
        Assert.equals(20, c.bar());
    }

    function test_assigning_to_a_more_specific_promoted() {
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
        final o = create_child();
        final v = pass_child(cast o);

        Assert.equals(10, v);
    }

    function test_passing_child_to_base_function() {
        final o = create_child();
        final v = pass_base(o);

        Assert.equals(10, v);
    }

    function test_reassigning() {
        var o : Base = create_child();

        Assert.equals(10, o.foo());

        o = create_child_as_child();

        Assert.equals(10, o.foo());
    }

    function create_child() : Base {
        return untyped __cpp__('new Child()');
    }

    function create_child_as_child() : Child {
        return untyped __cpp__('new Child()');
    }

    function pass_child(c : Child) {
        return c.foo();
    }

    function pass_base(b : Base) {
        return b.foo();
    }
}