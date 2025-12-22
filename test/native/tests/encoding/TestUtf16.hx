package tests.encoding;

import haxe.io.Bytes;
import cpp.encoding.Utf16;
import utest.Assert;
import utest.Test;

using cpp.marshal.ViewExtensions;

class TestUtf16 extends Test {
	function test_isEncoded_null() {
		Assert.raises(() -> Utf16.isEncoded(null));
	}

	function test_isEncoded_ascii() {
		Assert.isFalse(Utf16.isEncoded("test"));
	}

	function test_isEncoded_utf16() {
		Assert.isTrue(Utf16.isEncoded("😂"));
	}

	public function test_getByteCount_codepoint() {
		Assert.equals(2i64, Utf16.getByteCount('a'.code));
		Assert.equals(2i64, Utf16.getByteCount('ƅ'.code));
		Assert.equals(2i64, Utf16.getByteCount('バ'.code));
		Assert.equals(4i64, Utf16.getByteCount('𝄳'.code));
		Assert.equals(4i64, Utf16.getByteCount('😂'.code));
	}

	public function test_getByteCount_string_null() {
		Assert.raises(() -> Utf16.getByteCount((null:String)));
	}

	public function test_getByteCount_string_empty() {
		Assert.equals(0i64, Utf16.getByteCount(''));
	}

	public function test_getByteCount_string_ascii() {
		Assert.equals(26i64, Utf16.getByteCount('Hello, World!'));
	}

	public function test_getByteCount_string_utf16() {
		Assert.equals(26i64, Utf16.getByteCount('Hello😂World!'));
	}

	public function test_encode_codepoint() {
		final buffer = Bytes.alloc(4);

		Assert.equals(2i64, Utf16.encode('a'.code, buffer.asView()));
		Assert.equals(0x61, buffer.get(0));
		Assert.equals(0x00, buffer.get(1));
		buffer.asView().clear();

		Assert.equals(2i64, Utf16.encode('ƅ'.code, buffer.asView()));
		Assert.equals(0x85, buffer.get(0));
		Assert.equals(0x01, buffer.get(1));
		buffer.asView().clear();

		Assert.equals(2i64, Utf16.encode('バ'.code, buffer.asView()));
		Assert.equals(0xD0, buffer.get(0));
		Assert.equals(0x30, buffer.get(1));
		buffer.asView().clear();

		Assert.equals(4i64, Utf16.encode('𝄳'.code, buffer.asView()));
		Assert.equals(0x34, buffer.get(0));
		Assert.equals(0xD8, buffer.get(1));
		Assert.equals(0x33, buffer.get(2));
		Assert.equals(0xDD, buffer.get(3));
		buffer.asView().clear();
	}

	public function test_encode_codepoint_empty_view() {
		Assert.raises(() -> Utf16.encode('a'.code, ViewExtensions.empty()));
	}

	public function test_encode_codepoint_no_partial_writes() {
		final buffer = Bytes.alloc(2);

		Assert.raises(() -> Utf16.encode('𝄳'.code, buffer.asView()));
		Assert.equals(0, buffer.get(0));
		Assert.equals(0, buffer.get(1));
	}

	public function test_encode_string_null() {
		final buffer = Bytes.alloc(8);

		Assert.raises(() -> Utf16.encode((null:String), buffer.asView()));
	}

	public function test_encode_string_empty_view() {
		Assert.raises(() -> Utf16.encode('test', ViewExtensions.empty()));
	}

	public function test_encode_string_empty_string() {
		final buffer = Bytes.alloc(8);

		Assert.equals(0i64, Utf16.encode('', buffer.asView()));
	}

	public function test_encode_string_small_buffer() {
		final buffer = Bytes.alloc(2);

		Assert.raises(() -> Utf16.encode('test', buffer.asView()));
		Assert.equals(0, buffer.get(0));
		Assert.equals(0, buffer.get(1));
	}

	public function test_encode_string_ascii() {
		final buffer = Bytes.alloc(8);

		Assert.equals(8i64, Utf16.encode('test', buffer.asView()));
		Assert.equals('t'.code, buffer.get(0));
		Assert.equals(0, buffer.get(1));
		Assert.equals('e'.code, buffer.get(2));
		Assert.equals(0, buffer.get(3));
		Assert.equals('s'.code, buffer.get(4));
		Assert.equals(0, buffer.get(5));
		Assert.equals('t'.code, buffer.get(6));
		Assert.equals(0, buffer.get(7));
	}

	public function test_encode_string_utf16() {
		final buffer = Bytes.alloc(16);

		Assert.equals(12i64, Utf16.encode('te😂st', buffer.asView()));
		Assert.equals('t'.code, buffer.get(0));
		Assert.equals(0, buffer.get(1));
		Assert.equals('e'.code, buffer.get(2));
		Assert.equals(0, buffer.get(3));

		Assert.equals(0x3D, buffer.get(4));
		Assert.equals(0xD8, buffer.get(5));
		Assert.equals(0x02, buffer.get(6));
		Assert.equals(0xDE, buffer.get(7));

		Assert.equals('s'.code, buffer.get(8));
		Assert.equals(0, buffer.get(9));
		Assert.equals('t'.code, buffer.get(10));
		Assert.equals(0, buffer.get(11));
	}

	public function test_decode_codepoint() {
		var codepoint : cpp.Char32 = 0;

		var bytes = Bytes.ofHex('6100');
		Assert.equals(2i64, Utf16.decode(bytes.asView(), codepoint));
		Assert.equals('a'.code, cast codepoint);

		var bytes = Bytes.ofHex('8501');
		Assert.equals(2i64, Utf16.decode(bytes.asView(), codepoint));
		Assert.equals('ƅ'.code, cast codepoint);

		var bytes = Bytes.ofHex('D030');
		Assert.equals(2i64, Utf16.decode(bytes.asView(), codepoint));
		Assert.equals('バ'.code, cast codepoint);

		var bytes = Bytes.ofHex('34D833DD');
		Assert.equals(4i64, Utf16.decode(bytes.asView(), codepoint));
		Assert.equals('𝄳'.code, cast codepoint);
	}

	public function test_decode_string() {
		var bytes = Bytes.ofHex('6100');
		Assert.equals('a', Utf16.decode(bytes.asView()));

		var bytes = Bytes.ofHex('8501');
		Assert.equals('ƅ', Utf16.decode(bytes.asView()));

		var bytes = Bytes.ofHex('D030');
		Assert.equals('バ', Utf16.decode(bytes.asView()));

		var bytes = Bytes.ofHex('34D833DD');
		Assert.equals('𝄳', Utf16.decode(bytes.asView()));
	}

	public function test_decode_empty_view() {
		Assert.raises(() -> Utf16.decode(ViewExtensions.empty()));
	}
}