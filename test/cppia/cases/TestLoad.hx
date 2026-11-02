package cases;

import utest.Test;
import utest.Assert;

class TestLoad extends Test {
	function testNewOfUnlinkedClass() {
		final source = sys.io.File.getBytes('bin/unlinked.cppia');

		Assert.exception(() -> cpp.cppia.Module.fromData(source.getData()), String, (e) -> e == 'Bad link', 'Linked a new of a class the host does not have');
	}
}
