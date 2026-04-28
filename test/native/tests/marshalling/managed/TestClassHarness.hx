package tests.marshalling.managed;

import haxe.Constraints;
import utest.Assert;
import utest.Test;

private typedef TestType = {
    public var number : Int;

    public function multiply(input : Int) : Int;
}

@:generic private class Foo<T : Constructible<()->Void> & TestType> {
    public var o : T;

    public function new() {
        o = new T();
    }
}

@:generic class TestClassHarness<T : Constructible<()->Void> & TestType> extends Test {
    function test_construction() {
        final c = new Foo<T>();

        Assert.notNull(c.o);
    }

    function test_var_access() {
        final c = new Foo<T>();

        Assert.equals(0, c.o.number);
    }

    function test_var_mutation() {
        final c = new Foo<T>();

        c.o.number = 7;

        Assert.equals(7, c.o.number);
    }

    function test_reassignment() {
        final c = new Foo<T>();
        final o = new T();

        o.number = 100;

        c.o = o;

        Assert.equals(100, c.o.number);
    }

    function test_function_call() {
        final c = new Foo<T>();

        c.o.number = 7;

        Assert.equals(14, c.o.multiply(2));
    }
}