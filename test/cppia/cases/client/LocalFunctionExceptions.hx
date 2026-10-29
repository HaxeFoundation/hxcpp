package cases.client;

class DummyClass {
	public function run() {}
}

class LocalFunctionExceptions extends Test {
	static function staticFunction():Dynamic {
		throw 'Thrown from static';
	}

	function testLocalCallingStatic() {
		function localFunction() {
			staticFunction();
			throw 'Thrown from local';
		}

		// manual try/catch instead of Assert.exception so we
		// also test the raw control flow
		try {
			localFunction();
		} catch (e:Dynamic) {
			Assert.equals('Thrown from static', e, "Incorrect exception caught from local function call: " + e);
			return;
		}
		Assert.fail("No exception caught");
	}

	function testCatchWithinLocal() {
		function localFunction() {
			try {
				staticFunction();
			} catch (e:Dynamic) {
				Assert.equals('Thrown from static', e, "Incorrect exception caught from local function call: " + e);
				return;
			}
			Assert.fail("Exception from static function not caught");
		}

		try {
			localFunction();
		} catch (e) {
			Assert.fail('Exception leaked from local function: $e');
		}
	}

	function testCatchFromLocal() {
		function localFunction() {
			throw 'Thrown from local';
		}

		try {
			localFunction();
		} catch (e:Dynamic) {
			Assert.equals('Thrown from local', e, "Incorrect exception caught from local function call: " + e);
			return;
		}
		Assert.fail("No exception caught");
	}

	function testObjMethodOnReturn() {
		function localFunction() {
			(staticFunction() : Dynamic).run();
		}

		try {
			localFunction();
		} catch (e:Dynamic) {
			Assert.equals('Thrown from static', e, "Incorrect exception caught from local function call: " + e);
			return;
		}
		Assert.fail("No exception caught");
	}

	// this test has to be run from a static method to avoid obscuring the original failure
	static function testClassMethodOnReturn_impl() {
		function localFunction() {
			(staticFunction() : DummyClass).run();
		}

		try {
			localFunction();
		} catch (e:Dynamic) {
			Assert.equals('Thrown from static', e, "Incorrect exception caught from local function call: " + e);
			return;
		}
		Assert.fail("No exception caught");
	}

	function testClassMethodOnReturn() {
		testClassMethodOnReturn_impl();
	}

	// this test has to be run from a static method to avoid obscuring the original failure
	static function testHostClassMethodOnHostReturn_impl() {
		function localFunction() {
			(staticFunction() : Common).dummyMethod();
		}

		try {
			localFunction();
		} catch (e:Dynamic) {
			Assert.equals('Thrown from static', e, "Incorrect exception caught from local function call: " + e);
			return;
		}
		Assert.fail("No exception caught");
	}

	function testHostClassMethodOnHostReturn() {
		testHostClassMethodOnHostReturn_impl();
	}
}
