package tests.marshalling;

import cpp.Reference;

@:include('vector')
@:semantics(value)
@:cpp.ValueType({ type : 'vector', namespace : [ 'std' ] })
extern class StdVector<T> implements ArrayAccess<T> {
    @:overload(function ():Void {})
    function new(s:Int):Void;

    function size():Int;

    function resize(s:Int):Void;

    function at(s:Int):Reference<T>;

    function push_back(v:T):Void;
}