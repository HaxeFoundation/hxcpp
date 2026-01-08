package tests.marshalling.view;

import cpp.Int64;
import cpp.UInt32;
import cpp.Int16;
import cpp.Pointer;
import cpp.marshal.View;
import utest.Test;
import utest.Assert;
import tests.marshalling.Point;

using cpp.marshal.ViewExtensions;

class TestView extends Test {
	function test_reading_ints() {
		final buffer = [ for (i in 0...10) i + 1 ];
		final view   = buffer.asView();

		for (i in 0...10) {
			Assert.equals(i + 1, view[i]);
		}
	}

	function test_writing_ints() {
		final buffer = [ for (_ in 0...10) 0 ];
		final view   = buffer.asView();

		for (i in 0...10) {
			view[i] = i + 1;
		}

		Assert.same([ for (i in 0...buffer.length) i + 1 ], buffer);
	}

	function test_reading_value_types() {
		final buffer = new Point(0, 0);
		final view   = new View(Pointer.addressOf(buffer), 1);
		final point  = view[0];

		Assert.equals(0f64, point.x);
		Assert.equals(0f64, point.y);

		buffer.x = 200;
		buffer.y = 300;

		Assert.notEquals(buffer.x, point.x);
		Assert.notEquals(buffer.y, point.y);
	}

	function test_writing_value_types() {
		final buffer = new Point(0, 0);
		final view   = new View<Point>(Pointer.addressOf(buffer), 1);
		final point  = new Point();

		view[0] = point;

		Assert.equals(buffer.x, point.x);
		Assert.equals(buffer.y, point.y);

		point.x = 200;
		point.y = 300;

		Assert.notEquals(buffer.x, point.x);
		Assert.notEquals(buffer.y, point.y);
	}

	function test_reading_value_type_fields() {
		final point  = new Point();
		final view   = new View(Pointer.addressOf(point), 1);

		Assert.equals( 7f64, view[0].x);
		Assert.equals(26f64, view[0].y);
	}

	function test_writing_value_type_fields() {
		final point  = new Point();
		final view   = new View(Pointer.addressOf(point), 1);

		view[0].x =  8;
		view[0].y = 40;

		Assert.equals( 8f64, point.x);
		Assert.equals(40f64, point.y);
	}

	function test_reading_pointer_type() {
		final obj  = Context.create();
		final view = new View(Pointer.addressOf(obj), 1);

		Assert.isTrue(obj == view[0]);
	}

	function test_writing_pointer_type() {
		final obj  = Context.createNull();
		final view = new View(Pointer.addressOf(obj), 1);
		final next = Context.create();

		view[0] = next;

		Assert.isTrue(next == view[0]);
		Assert.isTrue(obj == view[0]);
	}

	function test_reading_pointer_type_field() {
		final obj  = Context.create();
		final view = new View(Pointer.addressOf(obj), 1);

		Assert.equals(7, view[0].number);
	}

	function test_writing_pointer_type_field() {
		final obj  = Context.create();
		final view = new View(Pointer.addressOf(obj), 1);

		view[0].number = 200;

		Assert.equals(200, obj.number);
	}

	function test_reading_error() {
		final buffer = [ for (i in 0...10) i + 1 ];

		Assert.raises(() -> final _ = buffer.asView()[-1], String);
		Assert.raises(() -> final _ = buffer.asView()[10], String);
		Assert.raises(() -> final _ = buffer.asView()[20], String);
	}

	function test_writing_error() {
		final buffer = [ for (i in 0...10) i + 1 ];

		Assert.raises(() -> buffer.asView()[-1] = 100, String);
		Assert.raises(() -> buffer.asView()[10] = 100, String);
		Assert.raises(() -> buffer.asView()[20] = 100, String);
	}

	function test_slice() {
		final buffer = [ for (i in 0...10) i + 1 ];
		final view   = buffer.asView();
		final index  = 3;
		final slice  = view.slice(index);

		if (Assert.equals(7i64, slice.length)) {
			for (i in 0...haxe.Int64.toInt(slice.length)) {
				Assert.equals(i + index + 1, slice[i]);
			}
		}
	}

	function test_slice_errors() {
		final buffer = [ for (i in 0...10) i + 1 ];

		Assert.raises(() -> buffer.asView().slice(-1), String);
		Assert.raises(() -> buffer.asView().slice(100), String);
		Assert.isTrue(buffer.asView().slice(10).isEmpty());
	}

	function test_slice_with_length() {
		final buffer = [ for (i in 0...10) i + 1 ];
		final view   = buffer.asView();
		final index  = 3;
		final length = 4;
		final slice  = view.slice(index, length);

		if (Assert.equals(haxe.Int64.ofInt(length), slice.length)) {
			for (i in 0...haxe.Int64.toInt(slice.length)) {
				Assert.equals(i + index + 1, slice[i]);
			}
		}
	}

	function test_slice_with_length_errors() {
		final buffer = [ for (i in 0...10) i + 1 ];

		Assert.raises(() -> buffer.asView().slice(-1, 5), String);
		Assert.raises(() -> buffer.asView().slice(5, -1), String);
		Assert.raises(() -> buffer.asView().slice(1, 100), String);
		Assert.raises(() -> buffer.asView().slice(100, 5), String);
		Assert.raises(() -> buffer.asView().slice(10, 5), String);
		Assert.isTrue(buffer.asView().slice(10, 0).isEmpty());
	}

	function test_clear() {
		final buffer = [ for (_ in 0...10) 8 ];
		final view   = buffer.asView();

		view.slice(2, 6).clear();

		Assert.same([ 8, 8, 0, 0, 0, 0, 0, 0, 8, 8 ], buffer);
	}

	function test_fill() {
		final buffer = [ for (_ in 0...10) 0 ];
		final view   = buffer.asView();
		final value  = 8;

		view.slice(2, 6).fill(value);

		Assert.same([ 0, 0, value, value, value, value, value, value, 0, 0 ], buffer);
	}

	function test_isEmpty() {
		Assert.isTrue(new View<Int>(null, 0).isEmpty());

		final buffer = [ for (_ in 0...10) 0 ];

		Assert.isFalse(buffer.asView().isEmpty());
	}

	function test_equality() {
		final buffer = [ for (_ in 0...10) 0 ];
		final view   = buffer.asView();

		final fst = view.slice(0);
		final snd = view.slice(0);

		Assert.isTrue(fst == snd);
		Assert.isFalse(fst != snd);

		final trd = view.slice(2);

		Assert.isFalse(fst == trd);
		Assert.isTrue(fst != trd);
	}

	function test_reinterpret_equal_size() {
		final buffer = [ for (_ in 0...10) 0 ];
		final view   = buffer.asView();
		final second : View<UInt32> = view.reinterpret();

		Assert.equals(view.length, second.length);
	}

	function test_reinterpret_to_smaller_type() {
		final buffer = [ for (_ in 0...10) 0 ];
		final view   = buffer.asView();
		final second : View<Int16> = view.reinterpret();

		Assert.equals(view.length * 2, second.length);
	}

	function test_reinterpret_to_larger_type() {
		final buffer = [ for (_ in 0...3) 0 ];
		final view   = buffer.asView();
		final second : View<Int64> = view.reinterpret();

		Assert.equals(1i64, second.length);
	}
	
	function test_reinterpret_to_larger_type_not_enough_length() {
		final buffer = [ 0 ];
		final view   = buffer.asView();
		final second : View<Int64> = view.reinterpret();

		Assert.equals(0i64, second.length);
	}

	function test_reinterpret_to_value_type() {
		final buffer = [ 0f64, 0f64, 0f64, 0f64 ];
		final view   = buffer.asView();
		final points = (view.reinterpret() : View<Point>);

		Assert.equals(2i64, points.length);
		Assert.equals(0f64, points[0].x);
		Assert.equals(0f64, points[0].y);

		points[0].x = 200;
		points[0].y = 300;

		Assert.equals(200f64, buffer[0]);
		Assert.equals(300f64, buffer[1]);

		points[0] = new Point();

		Assert.equals(7f64, buffer[0]);
		Assert.equals(26f64, buffer[1]);
	}

	function test_tryCopyTo() {
		final buffer = [ for (i in 0...10) i ];
		final biggerDst   = buffer.copy();
		final matchingDst = buffer.copy();
		final smallerDst  = buffer.copy();

		biggerDst.resize(20);
		smallerDst.resize(5);

		Assert.isTrue(buffer.asView().tryCopyTo(biggerDst.asView()));
		Assert.isTrue(buffer.asView().tryCopyTo(matchingDst.asView()));
		Assert.isFalse(buffer.asView().tryCopyTo(smallerDst.asView()));

		Assert.same(buffer, biggerDst.slice(0, 10));
		Assert.same(buffer, matchingDst);
		Assert.same(buffer.slice(0, smallerDst.length), smallerDst);

		final overlapped = buffer.copy();

		final regionA = overlapped.asView().slice(2, 5);
		final regionB = overlapped.asView().slice(4, 5);

		Assert.isTrue(regionB.tryCopyTo(regionA));
		Assert.same([0, 1, 4, 5, 6, 7, 8, 7, 8, 9], overlapped);
	}
}
