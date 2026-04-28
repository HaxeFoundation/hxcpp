package tests.marshalling.managed;

@:include('Managed.hpp')
@:cpp.ManagedType({type: 'standard_naming', namespace: ['foo', 'bar'], flags: [StandardNaming]})
extern class StandardNamingExtern {
	static var constNumber:Int;

	var number:Int;

	function new():Void;

	function multiply(input:Int):Int;

	static function create(number:Int):StandardNamingExtern;
}
