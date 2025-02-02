package tests.marshalling.managed;

import utest.Assert;
import utest.Test;

@:include('Managed.hpp')
@:cpp.ManagedType({ type : 'standard_naming_obj', namespace : [ 'foo', 'bar' ] })
extern class NonStandardNamingExtern {
    static var constNumber : Int;

    var number : Int;

    function new() : Void;

    function multiply(input : Int) : Int;

    static function create(number : Int) : NonStandardNamingExtern;
}

// @:include('Managed.hpp')
// @:cpp.ManagedType({ type : 'standard_naming', namespace : [ 'foo', 'bar' ], flags : [ StandardNaming ] })
// extern class StandardNamingExtern {
//     function doubleNumber(input : Int) : Int;

//     static function create() : StandardNamingExtern;
// }

// @:include('Managed.hpp')
// @:cpp.ManagedType({ namespace : [ 'hx' ] })
// extern class WithClosure {
//     function new() : Void;

//     @:native('ReturnSeven')
//     function returnSeven() : Int;
// }

class TestLocalNonStandardManagedClass extends Test {

    function test_null_object() {
        final o : NonStandardNamingExtern = null;

        Assert.isNull(o);
    }

    function test_construction() {
        final o = new NonStandardNamingExtern();

        Assert.notNull(o);
    }

    function test_equality() {
        final o1 = new NonStandardNamingExtern();
        final o2 = o1;

        Assert.equals(o1, o2);
    }

    function test_inequality() {
        final o1 = new NonStandardNamingExtern();
        final o2 = new NonStandardNamingExtern();

        Assert.notEquals(o1, o2);
    }

    function test_var_access() {
        final o = new NonStandardNamingExtern();

        Assert.equals(0, o.number);
    }

    function test_var_mutation() {
        final o = new NonStandardNamingExtern();

        o.number = 7;

        Assert.equals(7, o.number);
    }

    function test_static_var_access() {
        Assert.equals(300, NonStandardNamingExtern.constNumber);
    }

    function test_static_var_mutation() {
        NonStandardNamingExtern.constNumber = 200;

        Assert.equals(200, NonStandardNamingExtern.constNumber);

        NonStandardNamingExtern.constNumber = 300;
    }

    function test_function_call() {
        final o = new NonStandardNamingExtern();

        o.number = 7;

        Assert.equals(14, o.multiply(2));
    }

    function test_static_function_call() {
        final o = NonStandardNamingExtern.create(7);

        Assert.notNull(o);
    }

    function test_to_string() {
        final o = new NonStandardNamingExtern();

        Assert.equals("My Custom Managed Type", Std.string(o));
    }

    function test_null_access() {
        final o : NonStandardNamingExtern = null;

        Assert.raises(() -> o.number = 7);
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