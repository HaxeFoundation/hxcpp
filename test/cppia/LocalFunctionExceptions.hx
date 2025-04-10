enum Status {
	Ok;
	Error(message:String);
}

class DummyClass {
	public function run() {}
}

class LocalFunctionExceptions {
	static function staticFunction():Dynamic {
		throw 'Thrown from static';
	}

	public static function testLocalCallingStatic():Status {
		function localFunction() {
			staticFunction();
			throw 'Thrown from local';
		}

		try {
			localFunction();
		} catch (e:String) {
			if (e == 'Thrown from static') {
				return Ok;
			} else {
				return Error("Incorrect exception caught from local function call");
			}
		}

		return Error("No exception caught");
	}

	public static function testCatchWithinLocal():Status {
		function localFunction() {
			try {
				staticFunction();
			} catch (e:String) {
				if (e == 'Thrown from static') {
					return Ok;
				} else {
					return Error("Incorrect exception caught from local function call");
				}
			}
			return Error("Exception from static function not caught");
		}

		return try {
			localFunction();
		} catch (e) {
			Error('Exception leaked from local function: $e');
		};
	}

	public static function testCatchFromLocal():Status {
		function localFunction() {
			throw 'Thrown from local';
		}

		try {
			localFunction();
		} catch (e:String) {
			if (e == 'Thrown from local') {
				return Ok;
			} else {
				return Error("Incorrect exception caught from local function call");
			}
		}

		return Error("No exception caught");
	}

	public static function testObjMethodOnReturn():Status {
		function localFunction() {
			(staticFunction() : Dynamic).run();
		}

		try {
			localFunction();
		} catch (e:String) {
			if (e == 'Thrown from static') {
				return Ok;
			} else {
				return Error("Incorrect exception caught from local function call: " + e);
			}
		}

		return Error("No exception caught");
	}

	public static function testClassMethodOnReturn():Status {
		function localFunction() {
			(staticFunction() : DummyClass).run();
		}

		try {
			localFunction();
		} catch (e:String) {
			if (e == 'Thrown from static') {
				return Ok;
			} else {
				return Error("Incorrect exception caught from local function call: " + e);
			}
		}

		return Error("No exception caught");
	}

	public static function testHostClassMethodOnHostReturn():Status {
		function localFunction() {
			(staticFunction() : Common).dummyMethod();
		}

		try {
			localFunction();
		} catch (e:String) {
			if (e == 'Thrown from static') {
				return Ok;
			} else {
				return Error("Incorrect exception caught from local function call: " + e);
			}
		}

		return Error("No exception caught");
	}
}
