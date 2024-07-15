import utest.Test;
import utest.Assert;

class TestBasic extends Test
{
  function testStartTelemetry()
  {
    var thread_id:Int = startTelemetry(true, true);
    Assert.isTrue(thread_id>=0);
  }

  function startTelemetry(with_profiler:Bool=true,
                          with_allocations:Bool=true):Int
  {
    // Compile will fail without -D HXCPP_TELEMETRY
    return untyped __global__.__hxcpp_hxt_start_telemetry(with_profiler,
                                                          with_allocations);
  }

}
