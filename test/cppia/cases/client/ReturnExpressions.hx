package cases.client;

class ReturnExpressions extends Test {
	function setup() {
		Common.count = 0;
	}

	function teardown() {
		Common.count = 0;
	}

	// prevent the call from being optimised out due to unconditional return
	@:analyzer(ignore)
	static function returnAsCallHaxeThis() {
		(cast return:Common).dummyMethodArg(Common.incrementCount());
	}

	function testHostThisReturn() {
		try {
			returnAsCallHaxeThis();
			Assert.equals(0, Common.count, "Host method executed after return");
		} catch (e) {
			Assert.fail(e.toString());
		}
	}

	@:analyzer(ignore)
	static function returnAsCallHaxeArg() {
		new Common().instanceIncrementCount(return);
	}

	function testHostArgReturn() {
		try {
			returnAsCallHaxeArg();
			Assert.equals(0, Common.count, "Host method executed after return");
		} catch (e) {
			Assert.fail(e.toString());
		}
	}

	@:analyzer(ignore)
	static function returnAsFunction() {
		// if return is not handled, there is no function to run so this will
		// give a null function exception
		(cast return : (Int) -> Void)(Common.incrementCount());
	}

	function testFuncReturn() {
		try {
			returnAsFunction();
			Assert.equals(0, Common.count, "Host method executed after return");
		} catch (e) {
			Assert.fail(e.toString());
		}
	}

	function vtableMethod(_) {}

	@:analyzer(ignore)
	static function returnAsClientThis() {
		// if return is not handled, there is no instance to run the method with
		// so this will give a null function exception
		(cast return:ReturnExpressions).vtableMethod(Common.incrementCount());
	}

	function testClientThisReturn() {
		try {
			returnAsClientThis();
			Assert.equals(0, Common.count, "Host method executed after return");
		} catch (e) {
			Assert.fail(e.toString());
		}
	}
}
