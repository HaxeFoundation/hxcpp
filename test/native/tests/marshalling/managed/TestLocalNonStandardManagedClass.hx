package tests.marshalling.managed;

import utest.Assert;

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