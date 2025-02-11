enum Status {
	Ok;
	Error(message:String);
}

class LocalFunctionExceptions {
	static function staticFunction() {
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
}
