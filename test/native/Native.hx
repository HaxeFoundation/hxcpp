package;

class Native
{
	static function main()
	{
		utest.UTest.run([
			new tests.TestStdio(),
			new tests.TestRgb(),
			new tests.TestRectangle(),
			new tests.TestGlobalNamespace(),
			new tests.TestNativeGen(),
			new tests.TestNonVirtual(),
			new tests.TestPtr(),
			new tests.TestNativeEnum()
		]);
	}
}
