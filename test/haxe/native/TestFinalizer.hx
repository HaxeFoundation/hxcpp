package native;

@:native("ExternStruct")
extern class ExternStruct
{
   @:native("~ExternStruct")
   public function destroy():Void;

   @:native("new ExternStruct")
   public static function create():cpp.Pointer<ExternStruct>;
}

@:headerCode('
   struct ExternStruct {
      ExternStruct() {  }
      ~ExternStruct() {  }
   };
')
class ExternWrapper
{
    var pointer:cpp.Pointer<ExternStruct>;
    public static var instances:Int = 0;

    public function new()
    {
       pointer = ExternStruct.create();
       instances++;
       cpp.vm.Gc.setFinalizer(this, cpp.Function.fromStaticFunction(destroy));
    }

    @:void static public function destroy(ExternWrapper : ExternWrapper) : Void {
       instances--;
	 	 ExternWrapper.pointer.destroy();
    }
}

class TestFinalizer extends haxe.unit.TestCase
{
   public function testCount()
   {
      for(i in 0...10)
      {
          new ExternWrapper();
          cpp.vm.Gc.run(true);
      }
      assertTrue( ExternWrapper.instances < 10 );
   }
}
