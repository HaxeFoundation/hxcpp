package tests.marshalling;

@:semantics(value)
@:include('ctx.hpp')
@:cpp.PointerType({ type : 'ctx' })
extern class Context {
    public static function create() : Context;

    @:native('create_null')
    public static function createNull() : Context;

    var number : Int;

    @:native('Double')
    function double() : Int;
}