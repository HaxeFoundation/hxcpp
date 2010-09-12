/*
   This file contains temporary fixes that will be removed once the next version of haxe is out.

   You can use this with "-lib hxcpp" on the compile line

   Contains fixed for extra "__s"

 */
package cpp.io;

typedef FileHandle = Dynamic;


enum FileSeek {
	SeekBegin;
	SeekCur;
	SeekEnd;
}

/**
	API for reading and writing to files.
**/
class File {

	public static function getContent( path : String ) {
		var b = getBytes(path);
		return b.toString();
	}

	public static function getBytes( path : String ) : haxe.io.Bytes {
		var data:haxe.io.BytesData = file_contents(path);
		return haxe.io.Bytes.ofData(data);
	}

	public static function read( path : String, binary : Bool ) {
		return new FileInput(file_open(path,(if( binary ) "rb" else "r")));
	}

	public static function write( path : String, binary : Bool ) {
		return new FileOutput(file_open(path,(if( binary ) "wb" else "w")));
	}

	public static function append( path : String, binary : Bool ) {
		return new FileOutput(file_open(path,(if( binary ) "ab" else "a")));
	}

	public static function copy( src : String, dst : String ) {
		var s = read(src,true);
		var d = write(dst,true);
		d.writeInput(s);
		s.close();
		d.close();
	}

	public static function stdin() {
		return new FileInput(file_stdin());
	}

	public static function stdout() {
		return new FileOutput(file_stdout());
	}

	public static function stderr() {
		return new FileOutput(file_stderr());
	}

	public static function getChar( echo : Bool ) : Int {
		return getch(echo);
	}

	private static var file_stdin = cpp.Lib.load("std","file_stdin",0);
	private static var file_stdout = cpp.Lib.load("std","file_stdout",0);
	private static var file_stderr = cpp.Lib.load("std","file_stderr",0);

	private static var file_contents = cpp.Lib.load("std","file_contents",1);
	private static var file_open = cpp.Lib.load("std","file_open",2);

	private static var getch = cpp.Lib.load("std","sys_getch",1);

}
