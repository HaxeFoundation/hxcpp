package tests.marshalling.managed;

@:include('Managed.hpp')
@:cpp.ManagedType({type: 'standard_naming_obj', namespace: ['foo', 'bar']})
extern class NonStandardNamingExtern {
	static var constNumber:Int;

	var number:Int;

	function new():Void;

	function multiply(input:Int):Int;

	static function create(number:Int):NonStandardNamingExtern;
}
