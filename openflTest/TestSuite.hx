import massive.munit.TestSuite;
import hx.GcTest;
class TestSuite extends massive.munit.TestSuite {

	public function new() {
		super();
		add(hx.GcTest);
	}
}
