class HostBase implements IHostInterface
{
   static var hostInit = 10;

   
   var floatVal:Float;

   public function new()
   {
      floatVal = 1.25;
   }
   public function getVal() return floatVal;

   // IHostInteface 
   public function hostImplOnly(i:Int, s:String, f:Float) : String return i+s+f;
   public function whoStartedYou() return "HostBase";
   public function whoOverridesYou() return "No one";
}
