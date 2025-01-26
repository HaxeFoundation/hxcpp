package tests.marshalling;

@:semantics(reference)
@:include('ctx.hpp')
@:cpp.PointerType({ type : 'ctx' })
extern class Context {
    @:native('create')
    public static function create() : Context;

    @:native('create_null')
    public static function createNull() : Context;

    var number : Int;

    @:native('Double')
    function double() : Int;
}