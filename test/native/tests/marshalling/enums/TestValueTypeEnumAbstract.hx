package tests.marshalling.enums;

import utest.Assert;
import utest.Test;

@:semantics(value)
@:include('colour.hpp')
@:cpp.ValueType({ type : 'colour' })
private extern enum abstract Colour(Int) {
    @:native('red')
    var Red;

    @:native('green')
    var Green;

    @:native('blue')
    var Blue;
}

class TestValueTypeEnumAbstract extends Test {
    function test_switching_on_uncaptured_enum() {
        final e = Colour.Green;

        switch e {
            case Green:
                Assert.pass();
            default:
                Assert.fail('Expected "green"');
        }
    }

    function test_switching_on_captured_enum() {
        final e = Colour.Green;
        final f = () -> {
            return e;
        }

        f();

        switch e {
            case Red, Blue:
                Assert.fail('Expected "green"');
            case Green:
                Assert.pass();
        }
    }

    function test_uncaptured_equals() {
        final e = Colour.Green;

        Assert.isTrue(e == Colour.Green);
    }

    function test_uncaptured_not_equals() {
        final e = Colour.Green;

        Assert.isFalse(e != Colour.Green);
    }

    function test_promoted_equals() {
        final e = Colour.Green;
        final f = () -> {
            return e;
        }

        f();

        Assert.isTrue(e == Colour.Green);
    }

    function test_promoted_not_equals() {
        final e = Colour.Green;
        final f = () -> {
            return e;
        }

        f();

        Assert.isFalse(e != Colour.Green);
    }

    function test_from_underlying_type() {
        final i = 1;
        final e : Colour = cast i;

        Assert.isTrue(e == Colour.Green);
    }

    function test_to_underlying_type() {
        final e = Colour.Green;
        final i : Int = cast e;

        Assert.equals(1, i);
    }
}