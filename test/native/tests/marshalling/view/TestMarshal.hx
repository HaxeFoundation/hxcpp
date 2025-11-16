package tests.marshalling.view;

import cpp.Char;
import cpp.Char16;
import cpp.Pointer;
import cpp.marshal.View;
import haxe.io.Bytes;
import utest.Test;
import utest.Assert;
import tests.marshalling.Point;

using cpp.marshal.Marshal;
using cpp.marshal.ViewExtensions;

class TestMarshal extends Test {
	function test_write_int() {
		final storage = 0;
		final source  = new View(Pointer.addressOf(storage), 1);
		final value   = 200;
		
		source.asBytesView().write(value);

		Assert.equals(value, storage);
	}

	function test_read_int() {
		final storage = 200;
		final source  = new View(Pointer.addressOf(storage), 1);

		Assert.isTrue(storage == source.asBytesView().read());
	}

	function test_write_float() {
		final storage = 0f64;
		final source  = new View(Pointer.addressOf(storage), 1);
		final value   = 200.81;
		
		source.asBytesView().write(value);

		Assert.equals(value, storage);
	}

	function test_read_float() {
		final storage = 200.81;
		final source  = new View(Pointer.addressOf(storage), 1);

		Assert.isTrue(storage == source.asBytesView().read());
	}

	function test_write_bool() {
		final storage = false;
		final source  = new View(Pointer.addressOf(storage), 1);
		final value   = true;
		
		source.asBytesView().write(value);

		Assert.equals(value, storage);
	}

	function test_read_bool() {
		final storage = true;
		final source  = new View(Pointer.addressOf(storage), 1);

		Assert.isTrue(storage == source.asBytesView().read());
	}

	function test_write_value_type() {
		final storage = new Point(0, 0);
		final source  = new View(Pointer.addressOf(storage), 1);
		final value   = new Point();
		
		source.asBytesView().write(value);

		Assert.equals(value.x, storage.x);
		Assert.equals(value.y, storage.y);
	}

	function test_read_value_type() {
		final storage = new Point();
		final source  = new View(Pointer.addressOf(storage), 1);
		final value   = (source.asBytesView().read() : Point);
		
		Assert.equals(value.x, storage.x);
		Assert.equals(value.y, storage.y);
	}

	function test_write_pointer_type() {
		final storage = Context.createNull();
		final source  = new View(Pointer.addressOf(storage), 1);
		final value   = Context.create();
		
		source.asBytesView().write(value);

		Assert.isTrue(storage == value);
	}

	function test_read_pointer_type() {
		final storage = Context.create();
		final source  = new View(Pointer.addressOf(storage), 1);
		final value   = (source.asBytesView().read() : Context);
		
		Assert.isTrue(storage == value);
	}

	function test_ascii_string_to_utf8() {
		final source = "Hello, World!";
		final view   = source.toCharView();

		if (Assert.equals(source.length + 1, view.length)) {
			Assert.equals(view[ 0], "H".code);
			Assert.equals(view[ 1], "e".code);
			Assert.equals(view[ 2], "l".code);
			Assert.equals(view[ 3], "l".code);
			Assert.equals(view[ 4], "o".code);
			Assert.equals(view[ 5], ",".code);
			Assert.equals(view[ 6], " ".code);
			Assert.equals(view[ 7], "W".code);
			Assert.equals(view[ 8], "o".code);
			Assert.equals(view[ 9], "r".code);
			Assert.equals(view[10], "l".code);
			Assert.equals(view[11], "d".code);
			Assert.equals(view[12], "!".code);
			Assert.equals(view[13], 0);
		}
	}

	function test_ascii_string_to_utf8_buffer() {
		final source = "Hello, World!";
		final buffer = Bytes.ofHex("FFFFFFFFFFFFFFFFFFFFFFFFFFFF");
		final view   = new View(Pointer.ofArray(buffer.getData()), buffer.length).reinterpret();
		final count  = Marshal.toCharView(source, view);

		if (Assert.equals(source.length + 1, count)) {
			Assert.equals(view[ 0], "H".code);
			Assert.equals(view[ 1], "e".code);
			Assert.equals(view[ 2], "l".code);
			Assert.equals(view[ 3], "l".code);
			Assert.equals(view[ 4], "o".code);
			Assert.equals(view[ 5], ",".code);
			Assert.equals(view[ 6], " ".code);
			Assert.equals(view[ 7], "W".code);
			Assert.equals(view[ 8], "o".code);
			Assert.equals(view[ 9], "r".code);
			Assert.equals(view[10], "l".code);
			Assert.equals(view[11], "d".code);
			Assert.equals(view[12], "!".code);
			Assert.equals(view[13], 0);
		}
	}

	function test_emoji_string_to_utf8() {
		final source = "😂";
		final view   = source.toCharView();

		if (Assert.equals(5, view.length)) {
			Assert.equals((0xf0:Char), view[0]);
			Assert.equals((0x9f:Char), view[1]);
			Assert.equals((0x98:Char), view[2]);
			Assert.equals((0x82:Char), view[3]);
			Assert.equals(0, view[4]);
		}
	}

	function test_emoji_string_to_utf8_buffer() {
		final source = "😂";
		final buffer = Bytes.ofHex("FFFFFFFFFF");
		final view   = new View(Pointer.ofArray(buffer.getData()), buffer.length).reinterpret();
		final count  = Marshal.toCharView(source, view);

		if (Assert.equals(5, count)) {
			Assert.equals((0xf0:Char), view[0]);
			Assert.equals((0x9f:Char), view[1]);
			Assert.equals((0x98:Char), view[2]);
			Assert.equals((0x82:Char), view[3]);
			Assert.equals(0, view[4]);
		}
	}

	function test_ascii_string_to_utf16() {
		final source = "Hello, World!";
		final view   = source.toWideCharView();

		if (Assert.equals(source.length + 1, view.length)) {
			Assert.equals(view[ 0], "H".code);
			Assert.equals(view[ 1], "e".code);
			Assert.equals(view[ 2], "l".code);
			Assert.equals(view[ 3], "l".code);
			Assert.equals(view[ 4], "o".code);
			Assert.equals(view[ 5], ",".code);
			Assert.equals(view[ 6], " ".code);
			Assert.equals(view[ 7], "W".code);
			Assert.equals(view[ 8], "o".code);
			Assert.equals(view[ 9], "r".code);
			Assert.equals(view[10], "l".code);
			Assert.equals(view[11], "d".code);
			Assert.equals(view[12], "!".code);
			Assert.equals(view[13], 0);
		}
	}

	function test_ascii_string_to_utf16_buffer() {
		final source = "Hello, World!";
		final buffer = Bytes.ofHex("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
		final view   = new View(Pointer.ofArray(buffer.getData()), buffer.length).reinterpret();
		final count  = Marshal.toWideCharView(source, view);

		if (Assert.equals(count, view.length)) {
			Assert.equals(view[ 0], "H".code);
			Assert.equals(view[ 1], "e".code);
			Assert.equals(view[ 2], "l".code);
			Assert.equals(view[ 3], "l".code);
			Assert.equals(view[ 4], "o".code);
			Assert.equals(view[ 5], ",".code);
			Assert.equals(view[ 6], " ".code);
			Assert.equals(view[ 7], "W".code);
			Assert.equals(view[ 8], "o".code);
			Assert.equals(view[ 9], "r".code);
			Assert.equals(view[10], "l".code);
			Assert.equals(view[11], "d".code);
			Assert.equals(view[12], "!".code);
			Assert.equals(view[13], 0);
		}
	}

	function test_emoji_string_to_utf16() {
		final source = "😂";
		final view   = source.toWideCharView();

		if (Assert.equals(3, view.length)) {
			Assert.equals((0xD83D:Char16), view[0]);
			Assert.equals((0xDE02:Char16), view[1]);
			Assert.equals(0, view[2]);
		}
	}

	function test_emoji_string_to_utf16_buffer() {
		final source = "😂";
		final buffer = Bytes.ofHex("FFFFFFFFFFFFFFFF");
		final view   = new View(Pointer.ofArray(buffer.getData()), 3 * 2).reinterpret();
		final count  = Marshal.toWideCharView(source, view);

		if (Assert.equals(count, view.length)) {
			Assert.equals((0xD83D:Char16), view[0]);
			Assert.equals((0xDE02:Char16), view[1]);
			Assert.equals(0, view[2]);
		}
	}
}