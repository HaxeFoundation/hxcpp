package pointers;

import utest.Assert;
import utest.Test;

@:semantics(reference)
@:include('point.hpp')
@:cpp.PointerType({ type : 'point', namespace : [ 'hx', 'maths' ] })
private extern class Point {
    var x : Float;
    var y : Float;
}

@:semantics(reference)
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

    function create_holder() : Holder {
        return untyped __cpp__('::holder(::hx::maths::point(), ::hx::maths::point())');
    }

    function create_holder_with_null() : Holder {
        return untyped __cpp__('::holder()');
    }
}