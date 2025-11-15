package tests.marshalling.view;

import cpp.Int64;
import cpp.UInt32;
import cpp.Int16;
import cpp.Pointer;
import cpp.marshal.View;
import utest.Test;
import utest.Assert;
import tests.marshalling.Point;

class TestView extends Test {
	function test_reading_ints() {
		final buffer = [ for (i in 0...10) i + 1 ];
		final view   = new View(Pointer.ofArray(buffer), buffer.length);

		for (i in 0...10) {
			Assert.equals(i + 1, view[i]);
		}
	}

	function test_writing_ints() {
		final buffer = [ for (_ in 0...10) 0 ];
		final view   = new View(Pointer.ofArray(buffer), buffer.length);

		for (i in 0...10) {
			view[i] = i + 1;
		}

		Assert.same([ for (i in 0...buffer.length) i + 1 ], buffer);
	}

	function test_reading_value_types() {
		final buffer = new Point(0, 0);
		final view   = new View<Point>(Pointer.addressOf(buffer), 1);
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

	function test_slice() {
		final buffer = [ for (i in 0...10) i + 1 ];
		final view   = new View(Pointer.ofArray(buffer), buffer.length);
		final index  = 3;
		final slice  = view.slice(index);

		if (Assert.equals(7, slice.length)) {
			for (i in 0...slice.length) {
				Assert.equals(i + index + 1, slice[i]);
			}
		}
	}

	function test_slice_with_length() {
		final buffer = [ for (i in 0...10) i + 1 ];
		final view   = new View(Pointer.ofArray(buffer), buffer.length);
		final index  = 3;
		final length = 4;
		final slice  = view.slice(index, length);

		if (Assert.equals(length, slice.length)) {
			for (i in 0...slice.length) {
				Assert.equals(i + index + 1, slice[i]);
			}
		}
	}

	function test_clear() {
		final buffer = [ for (_ in 0...10) 8 ];
		final view   = new View(Pointer.ofArray(buffer), buffer.length);

		view.slice(2, 6).clear();

		Assert.same([ 8, 8, 0, 0, 0, 0, 0, 0, 8, 8 ], buffer);
	}

	function test_fill() {
		final buffer = [ for (_ in 0...10) 0 ];
		final view   = new View(Pointer.ofArray(buffer), buffer.length);
		final value  = 8;

		view.slice(2, 6).fill(value);

		Assert.same([ 0, 0, value, value, value, value, value, value, 0, 0 ], buffer);
	}

	function test_isEmpty() {
		Assert.isTrue(new View<Int>(null, 0).isEmpty());

		final buffer = [ for (_ in 0...10) 0 ];

		Assert.isFalse(new View(Pointer.ofArray(buffer), buffer.length).isEmpty());
	}

	function test_equality() {
		final buffer = [ for (_ in 0...10) 0 ];
		final view   = new View(Pointer.ofArray(buffer), buffer.length);

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
		final view   = new View(Pointer.ofArray(buffer), buffer.length);
		final second : View<UInt32> = view.reinterpret();

		Assert.equals(view.length, second.length);
	}

	function test_reinterpret_to_smaller_type() {
		final buffer = [ for (_ in 0...10) 0 ];
		final view   = new View(Pointer.ofArray(buffer), buffer.length);
		final second : View<Int16> = view.reinterpret();

		Assert.equals(view.length * 2, second.length);
	}

	function test_reinterpret_to_larger_type() {
		final buffer = [ for (_ in 0...3) 0 ];
		final view   = new View(Pointer.ofArray(buffer), buffer.length);
		final second : View<Int64> = view.reinterpret();

		Assert.equals(1, second.length);
	}
	
	function test_reinterpret_to_larger_type_not_enough_length() {
		final buffer = [ 0 ];
		final view   = new View(Pointer.ofArray(buffer), buffer.length);
		final second : View<Int64> = view.reinterpret();

		Assert.equals(0, second.length);
	}

	function test_reinterpret_to_value_type() {
		final buffer = [ 0f64, 0f64, 0f64, 0f64 ];
		final view   = new View(Pointer.ofArray(buffer), buffer.length);
		final points = (view.reinterpret() : View<Point>);

		Assert.equals(2, points.length);
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
}
