package tests.marshalling.classes;

import utest.Assert;
import utest.Test;

@:semantics(value)
@:include('point.hpp')
@:cpp.ValueType({ type : 'point', namespace : [ 'hx', 'maths' ] })
private extern class Point {
    var x : Float;
    var y : Float;

    @:overload(function(_x : Float, _y : Float) : Void {})
    function new();
}

@:semantics(value)
@:include('holder.hpp')
@:cpp.ValueType({ type : 'holder', namespace : [] })
private extern class Holder {
    var p1 : Point;
    var p2 : Point;
    var pPtr : Point;

    @:overload(function(_p1 : Point, _p2 : Point) : Void {})
    function new();

    function create() : Point;

    function get_static() : Point;
}

class TestValueTypeFields extends Test {
    function test_struct_with_default_construction() {
        final v = new Point();
        Assert.equals(7f64, v.x);
        Assert.equals(26f64, v.y);
    }

    function test_promoted_struct_with_default_construction() {
        final v = new Point();
        final f = () -> {
            return v.x;
        }

        f();

        Assert.equals(7f64, v.x);
        Assert.equals(26f64, v.y);
    }

    function test_accessing_inner_value_types() {
        final v = new Holder();
        Assert.equals(7f64, v.p1.x);
        Assert.equals(26f64, v.p1.y);
    }

    function test_accessing_inner_value_types_promoted() {
        final v = new Holder();
        final f = () -> {
            return v.p1.x;
        }

        f();

        Assert.equals(7f64, v.p1.x);
        Assert.equals(26f64, v.p1.y);
    }

    function test_copying_struct_inner_type() {
        final v = new Holder();
        final p = v.p1;

        p.x = 33f64;

        Assert.equals(7f64, v.p1.x);
        Assert.equals(33f64, p.x);
    }

    function test_copying_struct_inner_type_promoted() {
        final v = new Holder();
        final p = v.p1;
        final f = () -> {
            p.x = 33f64;
        }

        f();

        Assert.equals(7f64, v.p1.x);
        Assert.equals(33f64, p.x);
    }

    function test_assigning_struct_inner_type() {
        final p = new Point(33, 66);
        final v = new Holder();

        v.p1 = p;

        Assert.equals(33f64, v.p1.x);
        Assert.equals(66f64, v.p1.y);
    }

    function test_assigning_struct_inner_promoted() {
        final p = new Point();
        final v = new Holder();
        final f = () -> {
            p.x = 33;
            p.y = 66;
        }

        f();

        v.p1 = p;

        Assert.equals(33f64, v.p1.x);
        Assert.equals(66f64, v.p1.y);
    }

    function test_extern_function_returning_value() {
        final h = new Holder();
        final p = h.create();

        Assert.equals(h.p1.x + h.p2.x, p.x);
        Assert.equals(h.p1.y + h.p2.y, p.y);
    }

    // function test_extern_function_returning_pointer() {
    //     final h  = new Holder();
    //     final p1 = h.get_static();

    //     Assert.equals( 7f64, p1.x);
    //     Assert.equals(26f64, p1.y);

    //     final p2 = h.get_static();

    //     Assert.equals( 7f64, p2.x);
    //     Assert.equals(26f64, p2.y);

    //     h.get_static().x = 33;
    //     h.get_static().y = 66;

    //     final p3 = h.get_static();

    //     Assert.equals(33f64, p3.x);
    //     Assert.equals(66f64, p3.y);
    // }
}