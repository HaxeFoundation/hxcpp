@:include('vector')
@:semantics(reference)
@:cpp.ValueType({ type : 'vector', namespace : [ 'std' ] })
extern class StdVector<T> implements ArrayAccess<T> {
    @:overload(function ():Void {})
    function new(s:Int):Void;

    function size():Int;

    function resize(s:Int):Void;
}