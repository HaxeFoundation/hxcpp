class ClientIHostImpl implements IHostInterface
{
   public function new() { }

   public function hostImplOnly(i:Int, s:String, f:Float) : String return "client";
   public function whoStartedYou() : String return "client";
   public function whoOverridesYou() : String return "client";
}
