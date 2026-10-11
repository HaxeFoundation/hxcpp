import sys.thread.Lock;
import sys.thread.Thread;
import utest.Assert;
import utest.Test;

class TestAllocations extends Test
{
  // Keeps what the worker thread allocates reachable, so it is never
  // reclaimed after that thread's telemetry has been deleted
  static var tracked:Array<Dynamic> = [];

  // HXTAllocation skips allocations made inside a CFFI call. Skipping one
  // must leave alloc_mutex usable, or the next thread to lock it blocks.
  function testExternAllocationLeavesAllocationLockUsable()
  {
    var allocated = new Lock();
    Thread.create(function() {
      untyped __global__.__hxcpp_hxt_start_telemetry(true, true);
      tracked.push(allocateInExternFrame());
      tracked.push([4, 5, 6]);
      allocated.release();
    });
    Assert.isTrue(allocated.wait(10.0));

    // Stashing locks alloc_mutex, even without allocation tracking
    var stashed = new Lock();
    Thread.create(function() {
      untyped __global__.__hxcpp_hxt_stash_telemetry();
      stashed.release();
    });
    Assert.isTrue(stashed.wait(10.0));
  }

  // Primitives loaded with cpp.Lib.load run inside a frame like this one
  // (see ExternalPrimitive in src/hx/Lib.cpp), so what they allocate takes
  // the extern path in HXTAllocation
  static function allocateInExternFrame():Array<Int>
  {
    var result:Array<Int> = null;
    untyped __cpp__('{
      HX_STACK_FRAME(::hx::EXTERN_CLASS_NAME, "cffi", 0, "extern::cffi", __FILE__, __LINE__, 0)
      {0} = ::Array_obj< int >::__new(3, 3);
    }', result);
    return result;
  }
}
