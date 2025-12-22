package tests.encoding;

import haxe.io.Bytes;
import cpp.encoding.Utf8;
import utest.Assert;
import utest.Test;

using cpp.marshal.ViewExtensions;

class TestUtf8 extends Test {
	public function test_getByteCount_codepoint() {
		Assert.equals(1i64, Utf8.getByteCount('a'.code));
		Assert.equals(2i64, Utf8.getByteCount('ƅ'.code));
		Assert.equals(3i64, Utf8.getByteCount('バ'.code));
		Assert.equals(4i64, Utf8.getByteCount('𝄳'.code));
	}

	public function test_getByteCount_string_null() {
		Assert.raises(() -> Utf8.getByteCount((null:String)));
	}

	public function test_getByteCount_string_empty() {
		Assert.equals(0i64, Utf8.getByteCount(''));
	}

	public function test_getByteCount_string_ascii() {
		Assert.equals(13i64, Utf8.getByteCount('Hello, World!'));
	}

	public function test_getByteCount_string_utf16() {
		Assert.equals(15i64, Utf8.getByteCount('Hello😂World!'));
	}

	public function test_encode_codepoint() {
		final buffer = Bytes.alloc(4);

		Assert.equals(1i64, Utf8.encode('a'.code, buffer.asView()));
		Assert.equals(0x61, buffer.get(0));
		buffer.asView().clear();

		Assert.equals(2i64, Utf8.encode('ƅ'.code, buffer.asView()));
		Assert.equals(0xC6, buffer.get(0));
		Assert.equals(0x85, buffer.get(1));
		buffer.asView().clear();

		Assert.equals(3i64, Utf8.encode('バ'.code, buffer.asView()));
		Assert.equals(0xE3, buffer.get(0));
		Assert.equals(0x83, buffer.get(1));
		Assert.equals(0x90, buffer.get(2));
		buffer.asView().clear();

		Assert.equals(4i64, Utf8.encode('𝄳'.code, buffer.asView()));
		Assert.equals(0xF0, buffer.get(0));
		Assert.equals(0x9D, buffer.get(1));
		Assert.equals(0x84, buffer.get(2));
		Assert.equals(0xB3, buffer.get(3));
		buffer.asView().clear();
	}

	public function test_encode_codepoint_empty_view() {
		Assert.raises(() -> Utf8.encode('a'.code, ViewExtensions.empty()));
	}

	public function test_encode_codepoint_no_partial_writes() {
		final buffer = Bytes.alloc(2);

		Assert.raises(() -> Utf8.encode('𝄳'.code, buffer.asView()));
		Assert.equals(0, buffer.get(0));
		Assert.equals(0, buffer.get(1));
	}

	public function test_encode_string_null() {
		final buffer = Bytes.alloc(8);

		Assert.raises(() -> Utf8.encode((null:String), buffer.asView()));
	}

	public function test_encode_string_empty_view() {
		Assert.raises(() -> Utf8.encode('test', ViewExtensions.empty()));
	}

	public function test_encode_string_empty_string() {
		final buffer = Bytes.alloc(8);

		Assert.equals(0i64, Utf8.encode('', buffer.asView()));
	}

	public function test_encode_string_small_buffer() {
		final buffer = Bytes.alloc(2);

		Assert.raises(() -> Utf8.encode('test', buffer.asView()));
		Assert.equals(0, buffer.get(0));
		Assert.equals(0, buffer.get(1));
	}

	public function test_encode_string_ascii() {
		final buffer = Bytes.alloc(4);

		Assert.equals(4i64, Utf8.encode('test', buffer.asView()));
		Assert.equals('t'.code, buffer.get(0));
		Assert.equals('e'.code, buffer.get(1));
		Assert.equals('s'.code, buffer.get(2));
		Assert.equals('t'.code, buffer.get(3));
	}

	public function test_encode_string_utf16() {
		final buffer = Bytes.alloc(8);

		Assert.equals(8i64, Utf8.encode('te😂st', buffer.asView()));
		Assert.equals(0x74, buffer.get(0));
		Assert.equals(0x65, buffer.get(1));
		Assert.equals(0xF0, buffer.get(2));
		Assert.equals(0x9F, buffer.get(3));
		Assert.equals(0x98, buffer.get(4));
		Assert.equals(0x82, buffer.get(5));
		Assert.equals(0x73, buffer.get(6));
		Assert.equals(0x74, buffer.get(7));
	}

	public function test_decode_codepoint() {
		var codepoint : cpp.Char32 = 0;

		var bytes = Bytes.ofHex('61');
		Assert.equals(1i64, Utf8.decode(bytes.asView(), codepoint));
		Assert.equals('a'.code, cast codepoint);

		var bytes = Bytes.ofHex('c685');
		Assert.equals(2i64, Utf8.decode(bytes.asView(), codepoint));
		Assert.equals('ƅ'.code, cast codepoint);

		var bytes = Bytes.ofHex('e38390');
		Assert.equals(3i64, Utf8.decode(bytes.asView(), codepoint));
		Assert.equals('バ'.code, cast codepoint);

		var bytes = Bytes.ofHex('f09d84b3');
		Assert.equals(4i64, Utf8.decode(bytes.asView(), codepoint));
		Assert.equals('𝄳'.code, cast codepoint);
	}

	public function test_decode_string() {
		var bytes = Bytes.ofHex('61');
		Assert.equals('a', Utf8.decode(bytes.asView()));

		var bytes = Bytes.ofHex('c685');
		Assert.equals('ƅ', Utf8.decode(bytes.asView()));

		var bytes = Bytes.ofHex('e38390');
		Assert.equals('バ', Utf8.decode(bytes.asView()));

		var bytes = Bytes.ofHex('f09d84b3');
		Assert.equals('𝄳', Utf8.decode(bytes.asView()));
	}

	public function test_decode_empty_view() {
		Assert.raises(() -> Utf8.decode(ViewExtensions.empty()));
	}
}