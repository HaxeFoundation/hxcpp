package tests.marshalling.managed;

import haxe.Constraints;
import utest.Assert;
import utest.Test;

private typedef TestType = {
    public var number : Int;

    public function multiply(input : Int) : Int;
}

@:generic class TestLocalHarness<T : Constructible<()->Void> & TestType> extends Test {
    function test_null_object() {
        final o : T = null;

        Assert.isNull(o);
    }

    function test_construction() {
        final o = new T();

        Assert.notNull(o);
    }

    function test_equality() {
        final o1 = new T();
        final o2 = o1;

        Assert.equals(o1, o2);
    }

    function test_inequality() {
        final o1 = new T();
        final o2 = new T();

        Assert.notEquals(o1, o2);
    }

    function test_var_access() {
        final o = new T();

        Assert.equals(0, o.number);
    }

    function test_var_mutation() {
        final o = new T();

        o.number = 7;

        Assert.equals(7, o.number);
    }

    function test_function_call() {
        final o = new T();

        o.number = 7;

        Assert.equals(14, o.multiply(2));
    }

    function test_to_string() {
        final o = new T();

        Assert.equals("My Custom Managed Type", Std.string(o));
    }

    function test_null_access() {
        final o : T = null;

        Assert.raises(() -> o.number = 7);
    }

    function test_casting() {
        function create_as_any() : Any { 
            return new T();
        }

        final a = create_as_any();
        final o = (cast a : T);

        Assert.notNull(o);
    }

    function test_anon() {
        function create_anon() {
            return { o : new T() };
        }

        final a = create_anon();

        Assert.notNull(a.o);
    }

    // function test_type_check() {
    //     final o : Any = new NonStandardNamingExtern();

    //     Assert.isTrue(o is NonStandardNamingExtern);
    // }

    // function test_function_closures() {
    //     final o = new WithClosure();
    //     final f = o.returnSeven;

    //     Assert.equals(7, f());
    // }

    // function test_reflection() {
    //     //
    // }
}