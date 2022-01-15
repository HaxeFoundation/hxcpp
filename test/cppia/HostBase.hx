#if (hxcpp_api_level>=400)
import cpp.Native;
#end

class HostBase implements IHostInterface
{
   static var hostInit = 10;
   public static var hostBool0 = true;
   public static var hostBool1 = false;
   public static var hostBool2 = true;
   public static var hostBool3 = false;

   var floatVal:Float;
   var pointerSrc:cpp.Star<Int>;
   var pointerDest:cpp.Star<Int>;

   public function new()
   {
      floatVal = 1.25;
      #if (hxcpp_api_level>=400)
      pointerSrc = Native.malloc( Native.sizeof(Int) );
      Native.set(pointerSrc,4);
      pointerDest = null;
      #end
   }

   public function getDestVal() : Int
   {
      #if (hxcpp_api_level>=400)
      if (pointerDest==null)
         return -1;
      return Native.get(pointerDest);
      #else
      return 4;
      #end
   }

   public function getYou() : HostBase
   {
      return this;
   }

   public function testUpdateOverride() : Bool
   {
      return update()=="ClientExtends2 update";
   }


   public function getVal() return floatVal;

   public function getGeneration() return 0;

   public function update() return "HostBase update";

   // IHostInteface 
   public function hostImplOnly(i:Int, s:String, f:Float) : String return i+s+f;
   public function whoStartedYou() return "HostBase";
   public function whoOverridesYou() return "No one";
}
