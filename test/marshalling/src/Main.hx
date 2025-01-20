@:buildXml("<include name='${this_dir}/../src/Build.xml'/>")
class Main {
    static function main() {
        utest.UTest.run([
            new classes.TestLocalValueType(),
            new classes.TestClassValueType(),
            new classes.TestInterfaceValueType(),
            new classes.TestEnumValueType(),
            new classes.TestAbstractValueType(),
            new classes.TestValueTypeInterop(),
            new classes.TestValueTypeCollections(),
            new classes.TestValueTypeFields(),
            new classes.TestInheritance(),
            new enums.TestValueTypeEnumAbstract(),
            new enums.TestValueTypeEnumClassAbstract(),
            new pointers.TestLocalPointers(),
            new pointers.TestClassPointers(),
            new pointers.TestInterfacePointers(),
            new pointers.TestInheritancePointers(),
            new pointers.TestEnumPointers(),
            new pointers.TestPointerFields(),
            new pointers.TestPointerCollections(),
            new pointers.TestAbstractPointer()
        ]);
    }
}