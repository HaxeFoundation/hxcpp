class ClientExtends extends HostBase implements IClientInterface implements IClientHostInterface
{
   public function new()
   {
      super();
   }

   public function ok():Bool
   {
      return getVal()==1.25;
   }

   //override public function whoStartedYou() : String  return super.whoStartedYou();

   // override IHostInteface 
   override public function whoOverridesYou() return "ClientExtends";

   // new IClientInterface
   public function uniqueClientFunc() return "uniqueClientFunc";

   // IClientHostInterface
   public function whoAreYou() return "ClientExtends";

}
