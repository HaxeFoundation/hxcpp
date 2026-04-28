package tests.marshalling.enums;

import utest.Assert;
import utest.Test;

@:semantics(value)
@:include('Numbers.hpp')
@:cpp.ValueType({ namespace : [ 'foo' ] })
private extern enum abstract Numbers(Int) {
    var One;
    var Two;
    var Three;
}

class TestValueTypeEnumClassAbstract extends Test {
    function test_switching_on_uncaptured_enum() {
        final e = Numbers.Two;

        switch e {
            case Two:
                Assert.pass();
            default:
                Assert.fail('Expected "Two"');
        }
    }

    function test_switching_on_captured_enum() {
        final e = Numbers.Two;
        final f = () -> {
            return e;
        }

        f();

        switch e {
            case One, Three:
                Assert.fail('Expected "green"');
            case Two:
                Assert.pass();
        }
    }

    function test_uncaptured_equals() {
        final e = Numbers.Two;

        Assert.isTrue(e == Numbers.Two);
    }

    function test_uncaptured_not_equals() {
        final e = Numbers.Two;

        Assert.isFalse(e != Numbers.Two);
    }

    function test_promoted_equals() {
        final e = Numbers.Two;
        final f = () -> {
            return e;
        }

        f();

        Assert.isTrue(e == Numbers.Two);
    }

    function test_promoted_not_equals() {
        final e = Numbers.Two;
        final f = () -> {
            return e;
        }

        f();

        Assert.isFalse(e != Numbers.Two);
    }

    function test_from_underlying_type() {
        final i = 5;
        final e : Numbers = cast i;

        Assert.isTrue(e == Numbers.Two);
    }

    function test_to_underlying_type() {
        final e = Numbers.Two;
        final i : Int = cast e;

        Assert.equals(5, i);
    }
}