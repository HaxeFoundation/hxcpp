// hx::EnableJit, which Host.enableJit calls
@:cppInclude("hx/Scriptable.h")
class Main {
	static function main() {
		Inspect.caller("");
		var script = haxe.io.Path.join([haxe.io.Path.directory(Sys.programPath()), "script.cppia"]);
		for (jit in [false, true]) {
			cpp.cppia.Host.enableJit(jit);
			try {
				cpp.cppia.Host.runFile(script);
			} catch (e:Dynamic) {
				Sys.println("error: " + e);
			}
		}
	}
}
