package tests.marshalling.root;

import cpp.marshal.RootHandle;
import utest.Assert;
import utest.Test;

class TestRoot extends Test {
	function test_null_object() {
		Assert.raises(() -> RootHandle.create(null));
	}

	function test_null_void_pointer() {
		Assert.raises(() -> RootHandle.fromVoidPointer(null));
	}

	function test_get_object() {
		final obj    = "Hello, World!";
		final handle = RootHandle.create(obj);

		Assert.equals(obj, handle.getObject());

		handle.close();
	}

	function test_void_pointer_roundtrip() {
		final obj    = "Hello, World!";
		final ptr    = RootHandle.create(obj).toVoidPointer();
		final handle = RootHandle.fromVoidPointer(ptr);

		Assert.equals(obj, handle.getObject());

		handle.close();
	}
}