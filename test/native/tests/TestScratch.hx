package tests;

import cpp.Scratch;
import utest.Test;
import utest.Assert;

using haxe.Int64;

class TestScratch extends Test {
	function test_zeroed_small_alloc() {
		final size  = 16;
		final alloc = Scratch.alloc(size);

		Assert.equals(size, alloc.view.length.toInt());

		for (i in 0...size.toInt()) {
			Assert.equals(0, alloc.view[i]);
		}
	}

	function test_zeroed_large_alloc() {
		final size  = 100_000;
		final alloc = Scratch.alloc(size);

		Assert.equals(size, alloc.view.length.toInt());

		for (i in 0...size.toInt()) {
			Assert.equals(0, alloc.view[i]);
		}
	}

	function test_contents_wiped() {
		{
			final size  = 16;
			final alloc = Scratch.alloc(size);

			Assert.equals(size, alloc.view.length.toInt());

			alloc.view.fill(7);
		}

		{
			final size  = 16;
			final alloc = Scratch.alloc(size);

			Assert.equals(size, alloc.view.length.toInt());

			for (i in 0...size.toInt()) {
				Assert.equals(0, alloc.view[i]);
			}
		}
	}
}