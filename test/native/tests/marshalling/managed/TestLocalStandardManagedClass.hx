package tests.marshalling.managed;

import utest.Assert;
import utest.Test;

@:include('Managed.hpp')
@:cpp.ManagedType({ type : 'standard_naming', namespace : [ 'foo', 'bar' ], flags : [ StandardNaming ] })
extern class StandardNamingExtern {
    static var constNumber : Int;

    var number : Int;

    function new() : Void;

    function multiply(input : Int) : Int;

    static function create(number : Int) : StandardNamingExtern;
}

class TestLocalStandardManagedClass extends TestLocalHarness<StandardNamingExtern> {
    function test_static_var_access() {
        Assert.equals(300, StandardNamingExtern.constNumber);
    }

    function test_static_var_mutation() {
        StandardNamingExtern.constNumber = 200;

        Assert.equals(200, StandardNamingExtern.constNumber);

        StandardNamingExtern.constNumber = 300;
    }

    function test_static_function_call() {
        final o = StandardNamingExtern.create(7);

        Assert.notNull(o);
    }
}