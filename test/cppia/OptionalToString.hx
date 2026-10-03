/**
 * Scriptable hosts whose `toString` takes parameters, which hide `hx::Object::toString()`
 * in C++, and one whose `toString` takes none. The children inherit a hiding `toString`,
 * so the scriptable wrapper generated for them cannot call `toString()` on its parent.
 */
class OptionalToString {
	public function new() {}

	public function toString(indent:Int = 0):String {
		return "indent" + indent;
	}
}

class RequiredToString {
	public function new() {}

	public function toString(indent:Int, prefix:String):String {
		return prefix + indent;
	}
}

class ZeroArgToString {
	public function new() {}

	public function toString():String {
		return "zero";
	}
}

class OptionalToStringChild extends OptionalToString {}

class RequiredToStringChild extends RequiredToString {}
