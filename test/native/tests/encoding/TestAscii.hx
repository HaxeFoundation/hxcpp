package tests.encoding;

import haxe.io.Bytes;
import cpp.encoding.Ascii;
import utest.Assert;
import utest.Test;

using cpp.marshal.ViewExtensions;

class TestAscii extends Test
{
	function test_isEncoded_null() {
		Assert.raises(() -> Ascii.isEncoded(null));
	}

	function test_isEncoded_ascii() {
		Assert.isTrue(Ascii.isEncoded("test"));
	}

	function test_isEncoded_utf16() {
		Assert.isFalse(Ascii.isEncoded("😂"));
	}

	function test_encode_null() {
		final buffer = Bytes.alloc(4);

		Assert.raises(() -> Ascii.encode(null, buffer.asView()));
	}

	function test_encode_small_buffer() {
		final buffer = Bytes.alloc(2);

		Assert.raises(() -> Ascii.encode("test", buffer.asView()));
	}

	function test_encode_utf16() {
		final buffer = Bytes.alloc(1024);

		Assert.raises(() -> Ascii.encode("😂", buffer.asView()));
	}

	function test_encode() {
		final buffer = Bytes.alloc(1024);

		Assert.equals(4i64, Ascii.encode("test", buffer.asView()));
		Assert.equals('t'.code, buffer.get(0));
		Assert.equals('e'.code, buffer.get(1));
		Assert.equals('s'.code, buffer.get(2));
		Assert.equals('t'.code, buffer.get(3));
	}

	function test_decode_empty() {
		Assert.equals('', Ascii.decode(ViewExtensions.empty()));
	}

	function test_decode() {
		final buffer = Bytes.alloc(4);
		buffer.set(0, 't'.code);
		buffer.set(1, 'e'.code);
		buffer.set(2, 's'.code);
		buffer.set(3, 't'.code);
		
		Assert.equals('test', Ascii.decode(buffer.asView()));
	}
}