package tests.marshalling.pointers;

import utest.Assert;
import utest.Test;

@:semantics(value)
@:include('point.hpp')
@:cpp.PointerType({ type : 'point', namespace : [ 'hx', 'maths' ] })
private extern class Point {
    var x : Float;
    var y : Float;
}

@:semantics(value)
@:include('holder.hpp')
@:cpp.ValueType({ type : 'holder', namespace : [] })
private extern class Holder {
    var pPtr : Point;
}

class TestPointerFields extends Test {

    function test_field_access() {
        final h = create_holder();

        if (Assert.isTrue(h.pPtr != null)) {
            Assert.equals(45f64, h.pPtr.x);
            Assert.equals(67f64, h.pPtr.y);
        }
    }

    function test_field_null_access() {
        final h = create_holder_with_null();

        if (Assert.isTrue(h.pPtr == null)) {
            Assert.exception(() -> h.pPtr.x = 7);
        }
    }

    function test_copying_to_var() {
        final h = create_holder();
        final p = h.pPtr;

        p.x = 100;
        p.y = 200;

        Assert.equals(100f64, h.pPtr.x);
        Assert.equals(200f64, h.pPtr.y);
    }

    function test_assignment() {
        final h = create_holder();
        final p = create_point();

        h.pPtr = p;

        Assert.equals(100f64, h.pPtr.x);
        Assert.equals(200f64, h.pPtr.y);
    }

    function test_assignment_from_promoted() {
        final h = create_holder();
        final p = create_point();
        final f = () -> {
            return p.x;
        }

        f();

        h.pPtr = p;

        Assert.equals(100f64, h.pPtr.x);
        Assert.equals(200f64, h.pPtr.y);
    }

    function create_holder() : Holder {
        return untyped __cpp__('::holder(::hx::maths::point(), ::hx::maths::point())');
    }

    function create_holder_with_null() : Holder {
        return untyped __cpp__('::holder()');
    }

    function create_point() : Point {
        return untyped __cpp__('new ::hx::maths::point(100, 200)');
    }
}