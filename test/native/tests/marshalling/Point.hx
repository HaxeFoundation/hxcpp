package tests.marshalling;

@:semantics(value)
@:include('point.hpp')
@:cpp.ValueType({ type : 'point', namespace : [ 'hx', 'maths' ] })
extern class Point {
    var x : Float;
    var y : Float;

    @:overload(function(_x : Float, _y : Float) : Void {})
    function new();

    static function point_vec(v:StdVector<Point>):Void;
}