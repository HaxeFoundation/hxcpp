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

	@:analyzer(ignore)
	static function returnAsCallHaxeArg() {
		new Common().instanceIncrementCount(return);
	}

	public static function testHostArgReturn() {
		Common.count = 0;
		returnAsCallHaxeArg();
		if (Common.count == 0) {
			return Ok;
		}
		return Error("Host method executed after return");
	}

	@:analyzer(ignore)
	static function returnAsFunction() {
		// if return is not handled, there is no function to run so this will
		// give a null function exception
		(cast return : (Int) -> Void)(Common.incrementCount());
	}

	public static function testFuncReturn() {
		Common.count = 0;
		returnAsFunction();
		if (Common.count == 0) {
			return Ok;
		}
		return Error("Host method executed after return");
	}

	function vtableMethod(_) {}

	@:analyzer(ignore)
	public static function returnAsClientThis() {
		// if return is not handled, there is no instance to run the method with
		// so this will give a null function exception
		(cast return:ReturnExpressions).vtableMethod(Common.incrementCount());
	}

	public static function testClientThisReturn() {
		Common.count = 0;
		returnAsClientThis();
		if (Common.count == 0) {
			return Ok;
		}
		return Error("Host method executed after return");
	}
}
