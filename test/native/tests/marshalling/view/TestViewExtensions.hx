package tests.marshalling.view;

import haxe.io.UInt8Array;
import haxe.io.UInt16Array;
import haxe.io.UInt32Array;
import haxe.io.Int32Array;
import haxe.io.Float64Array;
import haxe.io.Float32Array;
import haxe.io.ArrayBufferView;
import haxe.io.Bytes;
import haxe.ds.Vector;
import cpp.Pointer;
import cpp.marshal.View;
import utest.Test;
import utest.Assert;

using cpp.marshal.ViewExtensions;

class TestViewExtensions extends Test {
	function test_toArray() {
		final source = 200;
		final view   = new View(Pointer.addressOf(source), 1);
		final array  = view.toArray();
		
		Assert.same([ source ], array);
	}

	function test_toVector() {
		final source = 200;
		final view   = new View(Pointer.addressOf(source), 1);
		final vector = view.toVector();
		
		if (Assert.equals(1, vector.length)) {
			Assert.equals(source, vector[0]);
		}
	}

	function test_toBytes() {
		final source = 200;
		final view   = new View(Pointer.addressOf(source), 1);
		final bytes  = view.toBytes();

		if (Assert.equals(4, bytes.length)) {
			Assert.equals(source, bytes.getInt32(0));
		}
	}

	function test_array_as_view() {
		final array = [ 100, 200, 300, 400 ];
		final view  = array.asView();

		if (Assert.equals(array.length, view.length)) {
			for (i in 0...array.length) {
				Assert.equals(array[i], view[i]);
			}
		}
	}

	function test_vector_as_view() {
		final vector = Vector.fromData([ 100, 200, 300, 400 ]);
		final view   = vector.asView();

		if (Assert.equals(vector.length, view.length)) {
			for (i in 0...vector.length) {
				Assert.equals(vector[i], view[i]);
			}
		}
	}

	function test_bytes_as_view() {
		final bytes = Bytes.ofData([ 10, 20, 30, 40 ]);
		final view  = bytes.asView();

		if (Assert.equals(bytes.length, view.length)) {
			for (i in 0...bytes.length) {
				Assert.equals(bytes.get(i), view[i]);
			}
		}
	}

	function test_array_buffer_view_as_view() {
		final index  = 7;
		final buffer = ArrayBufferView.fromBytes(Bytes.ofData([ for (i in 0...100) i ])).sub(index, 10);
		final view   = buffer.asView();

		if (Assert.equals(buffer.byteLength, view.length)) {
			for (i in 0...buffer.byteLength) {
				Assert.equals(buffer.buffer.get(index + i), view[i]);
			}
		}
	}

	function test_float32_array_as_view() {
		final index  = 7;
		final buffer = Float32Array.fromArray([ for (i in 0...100) i ]).sub(index, 10);
		final view   = buffer.asView();

		if (Assert.equals(buffer.length, view.length)) {
			for (i in 0...buffer.length) {
				Assert.equals(buffer[i], view[i]);
			}
		}
	}

	function test_float64_array_as_view() {
		final index  = 7;
		final buffer = Float64Array.fromArray([ for (i in 0...100) i ]).sub(index, 10);
		final view   = buffer.asView();

		if (Assert.equals(buffer.length, view.length)) {
			for (i in 0...buffer.length) {
				Assert.equals(buffer[i], view[i]);
			}
		}
	}

	function test_int32_array_as_view() {
		final index  = 7;
		final buffer = Int32Array.fromArray([ for (i in 0...100) i ]).sub(index, 10);
		final view   = buffer.asView();

		if (Assert.equals(buffer.length, view.length)) {
			for (i in 0...buffer.length) {
				Assert.equals(buffer[i], view[i]);
			}
		}
	}

	function test_uint32_array_as_view() {
		final index  = 7;
		final buffer = UInt32Array.fromArray([ for (i in 0...100) i ]).sub(index, 10);
		final view   = buffer.asView();

		if (Assert.equals(buffer.length, view.length)) {
			for (i in 0...buffer.length) {
				Assert.equals(buffer[i], view[i]);
			}
		}
	}

	function test_uint16_array_as_view() {
		final index  = 7;
		final buffer = UInt16Array.fromArray([ for (i in 0...100) i ]).sub(index, 10);
		final view   = buffer.asView();

		if (Assert.equals(buffer.length, view.length)) {
			for (i in 0...buffer.length) {
				Assert.equals(buffer[i], view[i]);
			}
		}
	}

	function test_uint8_array_as_view() {
		final index  = 7;
		final buffer = UInt8Array.fromArray([ for (i in 0...100) i ]).sub(index, 10);
		final view   = buffer.asView();

		if (Assert.equals(buffer.length, view.length)) {
			for (i in 0...buffer.length) {
				Assert.equals(buffer[i], view[i]);
			}
		}
	}
}