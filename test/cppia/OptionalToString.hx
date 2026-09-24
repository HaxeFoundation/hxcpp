/**
 * A scriptable host whose `toString` takes an optional argument.
 *
 * hxcpp emits that as `toString(hx::Null<int>)`. The scriptable wrapper used to call
 * `toString()` with no arguments, which does not compile. `ZeroArgToString` keeps the
 * zero-argument form compiling too.
 */
class OptionalToString {
	public function new() {}

	public function toString(indent:Int = 0):String {
		return "indent" + indent;
	}
}

class ZeroArgToString {
	public function new() {}

	public function toString():String {
		return "zero";
	}
}
