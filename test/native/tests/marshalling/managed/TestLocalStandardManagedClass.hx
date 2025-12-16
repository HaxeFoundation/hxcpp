package tests.marshalling.managed;

import utest.Assert;

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