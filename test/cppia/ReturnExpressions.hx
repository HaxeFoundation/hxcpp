import LocalFunctionExceptions.Status;

class ReturnExpressions {
	// prevent the call from being optimised out due to unconditional return
	@:analyzer(ignore)
	static function returnAsCallHaxeThis() {
		(cast return:Common).dummyMethodArg(Common.incrementCount());
	}

	public static function testHostThisReturn() {
		Common.count = 0;
		returnAsCallHaxeThis();
		if (Common.count == 0) {
			return Ok;
		}
		return Error("Host method executed after return");
	}
}
