class Script {
	var label:String;

	function new()
		label = "probe";

	function probe(a:Int, b:String):String {
		var c = a * 2;
		var seen = Inspect.caller("probe");
		return seen + (c > 0 ? "" : "");
	}

	function mixed(i:Int, f:Float, s:String, j:Int, o:Array<Int>):String {
		var k = i + j;
		var seen = Inspect.caller("mixed");
		return seen + (k > 0 && f > 0 && s != null && o != null ? "" : "");
	}

	function defaults(a:Int, b:Int = 7, ?c:String):String {
		var seen = Inspect.caller("defaults");
		return seen + (a + b > 0 && c == null ? "" : "");
	}

	function setter(n:Int):String {
		var m = n;
		Inspect.set("setter", "m", 99);
		Inspect.set("setter", "n", 5);
		return "m=" + m + " n=" + n;
	}

	static function staticProbe(x:Float):String {
		var y = x + 1;
		var seen = Inspect.caller("staticProbe");
		return seen + (y > 0 ? "" : "");
	}

	static function main() {
		var s = new Script();
		Sys.println("method: " + s.probe(21, "two"));
		Sys.println("mixed: " + s.mixed(1, 2.5, "s", 3, [4]));
		Sys.println("defaults: " + s.defaults(1));
		Sys.println("dynamic: " + Reflect.callMethod(s, Reflect.field(s, "mixed"), [5, 0.5, "d", 6, []]));
		Sys.println("setter: " + s.setter(1));
		Sys.println("static: " + staticProbe(1.5));
	}
}
