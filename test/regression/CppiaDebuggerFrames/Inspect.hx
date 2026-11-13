import cpp.vm.Debugger;

class Inspect {
	static function frame(name:String):Int {
		var thread = Debugger.getCurrentThreadNumber();
		var stack = Debugger.getThreadInfo(thread, true).stack;
		for (i in 0...stack.length)
			if (stack[i].functionName == name)
				return i;
		return -1;
	}

	public static function caller(name:String):String {
		var thread = Debugger.getCurrentThreadNumber();
		var i = frame(name);
		if (i < 0)
			return "no frame for " + name;
		var names = Debugger.getStackVariables(thread, i, true);
		names.sort(Reflect.compare);
		var values = [for (n in names) n == "this" ? "this=" + Std.string(Debugger.getStackVariableValue(thread, i, n, true) != null)
			: n + "=" + Std.string(Debugger.getStackVariableValue(thread, i, n, true))];
		return values.join(" ");
	}

	public static function set(name:String, variable:String, value:Dynamic) {
		Debugger.setStackVariableValue(Debugger.getCurrentThreadNumber(), frame(name), variable, value, true);
	}
}
