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

class TestLocalNonStandardManagedClass extends TestLocalHarness<NonStandardNamingExtern> {
    function test_static_var_access() {
        Assert.equals(300, NonStandardNamingExtern.constNumber);
    }

    function test_static_var_mutation() {
        NonStandardNamingExtern.constNumber = 200;

        Assert.equals(200, NonStandardNamingExtern.constNumber);

        NonStandardNamingExtern.constNumber = 300;
    }

    function test_static_function_call() {
        final o = NonStandardNamingExtern.create(7);

        Assert.notNull(o);
    }
}