package tests.marshalling.view;

import haxe.ds.Vector;
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

	function test_write_not_enough_space() {
		final storage = Bytes.alloc(1);

		Assert.raises(() -> storage.asView().writeInt32(0));
	}

	function test_read_int() {
		final storage = 200;
		final source  = new View(Pointer.addressOf(storage), 1);

		Assert.isTrue(storage == source.asBytesView().read());
	}

	function test_read_not_enough_space() {
		final storage = Bytes.alloc(1);

		Assert.raises(() -> storage.asView().readInt32());
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
		final view   = buffer.asView().reinterpret();
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
		final view   = buffer.asView().reinterpret();
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
		final view   = buffer.asView().reinterpret();
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
		final view   = buffer.asView().slice(0, 3 * 2).reinterpret();
		final count  = Marshal.toWideCharView(source, view);

		if (Assert.equals(count, view.length)) {
			Assert.equals((0xD83D:Char16), view[0]);
			Assert.equals((0xDE02:Char16), view[1]);
			Assert.equals(0, view[2]);
		}
	}

	function test_ascii_chars_to_string() {
		final buffer = new Vector<Char>(5);
		buffer[0] = 'H'.code;
		buffer[1] = 'e'.code;
		buffer[2] = 'l'.code;
		buffer[3] = 'l'.code;
		buffer[4] = 'o'.code;
		final view   = buffer.asView();
		final string = view.toString();

		Assert.equals('Hello', string);
	}

	function test_ascii_wide_chars_to_string() {
		final buffer = new Vector<Char16>(5);
		buffer[0] = 'H'.code;
		buffer[1] = 'e'.code;
		buffer[2] = 'l'.code;
		buffer[3] = 'l'.code;
		buffer[4] = 'o'.code;
		final view   = buffer.asView();
		final string = view.toString();

		Assert.equals('Hello', string);
	}

	function test_null_terminated_ascii_chars_to_string() {
		final buffer = new Vector<Char>(5);
		buffer[0] = 'H'.code;
		buffer[1] = 'e'.code;
		buffer[2] = 'l'.code;
		buffer[3] = 'l'.code;
		buffer[4] = 'o'.code;
		buffer[5] = 0;
		final view   = buffer.asView();
		final string = view.toString();

		Assert.equals('Hello', string);
	}
	
	function test_null_terminated_ascii_wide_chars_to_string() {
		final buffer = new Vector<Char16>(5);
		buffer[0] = 'H'.code;
		buffer[1] = 'e'.code;
		buffer[2] = 'l'.code;
		buffer[3] = 'l'.code;
		buffer[4] = 'o'.code;
		buffer[5] = 0;
		final view   = buffer.asView();
		final string = view.toString();

		Assert.equals('Hello', string);
	}

	function test_utf8_bytes_to_string() {
		final buffer = Bytes.ofHex("f09f9882");
		final view   = (buffer.asView().reinterpret() : View<Char>);
		final string = view.toString();

		Assert.equals('😂', string);
	}

	function test_null_terminated_utf8_bytes_to_string() {
		final buffer = Bytes.ofHex("f09f98820000");
		final view   = (buffer.asView().reinterpret() : View<Char>);
		final string = view.toString();

		Assert.equals('😂', string);
	}

	function test_utf16_bytes_to_string() {
		final buffer = Bytes.ofHex("3DD802De");
		final view   = (buffer.asView().reinterpret() : View<Char16>);
		final string = view.toString();

		Assert.equals('😂', string);
	}

	function test_null_terminated_utf16_bytes_to_string() {
		final buffer = Bytes.ofHex("3DD802De00000000");
		final view   = (buffer.asView().reinterpret() : View<Char16>);
		final string = view.toString();

		Assert.equals('😂', string);
	}
}