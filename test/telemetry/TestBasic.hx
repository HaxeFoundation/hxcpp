class TestBasic extends haxe.unit.TestCase
{
  public function new() super();

  function testStartTelemetry(string:String)
  {
    var thread_id:Int = startTelemetry(true, true);
    assertTrue(thread_id>=0);
  }

  function startTelemetry(with_profiler:Bool=true,
                          with_allocations:Bool=true):Int
  {
    // Compile will fail without -D HXCPP_TELEMETRY
    return untyped __global__.__hxcpp_hxt_start_telemetry(with_profiler,
                                                          with_allocations);
  }

}
