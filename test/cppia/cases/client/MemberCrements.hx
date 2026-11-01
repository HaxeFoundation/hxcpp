package cases.client;

private class Counter {
	public var n = 0;

	public function new() {}

	// Each of these only touches `this` through the increment/decrement itself

	public function postInc() {
		n++;
	}

	public function preInc() {
		++n;
	}

	public function postDec() {
		n--;
	}

	public function preDec() {
		--n;
	}

	public function postIncValue() {
		return n++;
	}

	public function preIncValue() {
		return ++n;
	}
}

class MemberCrements extends Test {
	function testVoid() {
		final c = new Counter();
		c.postInc();
		Assert.equals(1, c.n);
		c.preInc();
		Assert.equals(2, c.n);
		c.postDec();
		Assert.equals(1, c.n);
		c.preDec();
		Assert.equals(0, c.n);
	}

	function testValue() {
		final c = new Counter();
		Assert.equals(0, c.postIncValue());
		Assert.equals(1, c.n);
		Assert.equals(2, c.preIncValue());
		Assert.equals(2, c.n);
	}
}
