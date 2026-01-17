package;

#if (haxe_ver>=5)
@:buildXml("<include name='${this_dir}/../tests/marshalling/Build.xml'/>")
#end
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
			new tests.TestNativeEnum(),

			#if (haxe_ver>=5)
            new tests.marshalling.classes.TestLocalValueType(),
            new tests.marshalling.classes.TestClassValueType(),
            new tests.marshalling.classes.TestInterfaceValueType(),
            new tests.marshalling.classes.TestEnumValueType(),
            new tests.marshalling.classes.TestAbstractValueType(),
            new tests.marshalling.classes.TestValueTypeInterop(),
            new tests.marshalling.classes.TestValueTypeCollections(),
            new tests.marshalling.classes.TestValueTypeFields(),
            new tests.marshalling.classes.TestInheritance(),
            new tests.marshalling.enums.TestValueTypeEnumAbstract(),
            new tests.marshalling.enums.TestValueTypeEnumClassAbstract(),
            new tests.marshalling.pointers.TestLocalPointers(),
            new tests.marshalling.pointers.TestClassPointers(),
            new tests.marshalling.pointers.TestInterfacePointers(),
            new tests.marshalling.pointers.TestInheritancePointers(),
            new tests.marshalling.pointers.TestEnumPointers(),
            new tests.marshalling.pointers.TestPointerFields(),
            new tests.marshalling.pointers.TestPointerCollections(),
            new tests.marshalling.pointers.TestAbstractPointer(),
            new tests.marshalling.pointers.TestPointerInterop(),
            new tests.marshalling.managed.TestLocalNonStandardManagedClass(),
            new tests.marshalling.managed.TestLocalStandardManagedClass(),
            new tests.marshalling.managed.TestClassNonStandardManagedClass(),
            new tests.marshalling.managed.TestClassStandardManagedClass(),

            new tests.marshalling.view.TestView(),
            new tests.marshalling.view.TestMarshal(),
            new tests.marshalling.view.TestViewExtensions(),

            new tests.marshalling.root.TestRoot(),

            new tests.encoding.TestAscii(),
            new tests.encoding.TestUtf8(),
            new tests.encoding.TestUtf16(),
			#end
		]);
	}
}
