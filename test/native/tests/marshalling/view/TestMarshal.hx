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

	function test_asCharView_null() {
		Assert.raises(() -> Marshal.asCharView(null));
	}

	function test_asWideCharView_null() {
		Assert.raises(() -> Marshal.asWideCharView(null));
	}

	function test_asCharView_wrong_encoding() {
		Assert.raises(() -> Marshal.asCharView("😂"));
	}

	function test_asWideCharView_wrong_encoding() {
		Assert.raises(() -> Marshal.asWideCharView("hello"));
	}

	function test_asCharView() {
		final view = "hello".asCharView();
		
		Assert.equals(5i64, view.length);
	}

	function test_asWideCharView() {
		final view = "😂".asWideCharView();
		
		Assert.equals(2i64, view.length);
	}
}