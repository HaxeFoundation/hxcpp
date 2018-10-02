package native;

@:native("ExternStruct")
extern class ExternStruct
{
   @:native("~ExternStruct")
   public function destroy():Void;

   @:native("new ExternStruct")
   public static function create():cpp.Pointer<ExternStruct>;
}


#if !cppia
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


class CustomFinalizable
{
   public static var count = 0;

   public function new()
   {
      cpp.vm.Gc.setFinalizer(this, cpp.Function.fromStaticFunction(__finalizeCallback));
   }

   function __finalize() : Void count++;

   @:void public static function __finalizeCallback(o : CustomFinalizable) : Void
   {
      if(o != null)
         o.__finalize();
      else
         Sys.println("Null callback object?");
   }
} 



#end


class MyFinalizable extends cpp.Finalizable
{
   public static var count = 0;

   public function new()
   {
      super();
   }

   override public function finalize()
   {
      count ++;
   }
}


class TestFinalizer extends haxe.unit.TestCase
{

   override public function setup()
   {
      MyFinalizable.count = 0;
      CustomFinalizable.count = 0;
   }

   #if !cppia
   function createExternWrapper(i:Int)
   {
      if (i<=0)
         new ExternWrapper();
      else
        createExternWrapper(i-1);
   }


   public function testCount()
   {
      for(i in 0...10)
      {
          createExternWrapper(2);
          cpp.vm.Gc.run(true);
      }
      Sys.println("\nExtern instances remaining:" + ExternWrapper.instances);
      assertTrue( ExternWrapper.instances < 10 );
   }

   function createCustomFinalizable(i:Int)
   {
      if (i<=0)
         new CustomFinalizable();
      else
         createCustomFinalizable(i-1);
   }


   public function testCustomFinalizable()
   {
      for(i in 0...100)
         createCustomFinalizable(2);
      cpp.vm.Gc.run(true);
      Sys.println("custom cleared:" + CustomFinalizable.count);
      assertTrue(CustomFinalizable.count>0);
   }

   #end

   function createMyFinalizable(i:Int)
   {
      if (i<=0)
         new MyFinalizable();
      else
         createMyFinalizable(i-1);
   }

   public function testFinalizable()
   {
      for(i in 0...100)
         createMyFinalizable(2);
      cpp.vm.Gc.run(true);
      Sys.println("MyFinalizable cleared:" + MyFinalizable.count);
      assertTrue(MyFinalizable.count>0);
   }
}


