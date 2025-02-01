package tests.marshalling.managed;

import utest.Assert;
import utest.Test;

@:include('Managed.hpp')
@:cpp.ManagedType({ type : 'fooextern', namespace : [ 'hx' ] })
extern class FooExtern {
    static var constNumber : Int;

    var number : Int;

    function new() : Void;

    function doubleNumber() : Int;

    static function create(number : Int) : FooExtern;
}

@:include('Managed.hpp')
@:cpp.ManagedType({ namespace : [ 'foo', 'bar' ], flags : [ StandardNaming ] })
extern class StandardLayoutExtern {
    function doubleNumber(input : Int) : Int;

    static function create() : StandardLayoutExtern;
}

@:include('Managed.hpp')
@:cpp.ManagedType({ namespace : [ 'hx' ] })
extern class WithClosure {
    function new() : Void;

    @:native('ReturnSeven')
    function returnSeven() : Int;
}

class TestLocalManagedClass extends Test {

    function test_null_object() {
        final o : FooExtern = null;

        Assert.isNull(o);
    }

    function test_construction() {
        final o = new FooExtern();

        Assert.notNull(o);
    }

    function test_equality() {
        final o1 = new FooExtern();
        final o2 = o1;

        Assert.equals(o1, o2);
    }

    function test_inequality() {
        final o1 = new FooExtern();
        final o2 = new FooExtern();

        Assert.notEquals(o1, o2);
    }

    function test_var_access() {
        final o = new FooExtern();

        Assert.equals(0, o.number);
    }

    function test_var_mutation() {
        final o = new FooExtern();

        o.number = 7;

        Assert.equals(7, o.number);
    }

    function test_static_var_access() {
        Assert.equals(300, FooExtern.constNumber);
    }

    function test_static_var_mutation() {
        FooExtern.constNumber = 200;

        Assert.equals(200, FooExtern.constNumber);

        FooExtern.constNumber = 300;
    }

    function test_function_call() {
        final o = new FooExtern();

        o.number = 7;

        Assert.equals(14, o.doubleNumber());
    }

    function test_static_function_call() {
        final o = FooExtern.create(7);

        Assert.notNull(o);
    }

    function test_to_string() {
        final o = new FooExtern();

        Assert.equals("fooextern", Std.string(o));
    }

    function test_standard_naming() {
        final o = StandardLayoutExtern.create();

        Assert.equals(14, o.doubleNumber(7));
    }

    function test_null_access() {
        final o : FooExtern = null;

        Assert.raises(() -> o.number = 7);
    }

    function test_function_closures() {
        final o = new WithClosure();
        final f = o.returnSeven;

        Assert.equals(7, f());
    }

    // function test_reflection() {
    //     //
    // }
}