class Main {
	static function main() {
		var array:Array<Int> = [];
		cpp.Pointer.ofArray(array);
		trace(array.length);
	}
}
