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

   public function testOne()
   {
      return getOne()==1;
   }

   public function testOneExtended()
   {
      return getOne()==111;
   }


   #if (hxcpp_api_level>=400)
   public function testPointers() : Bool
   {
      pointerDest = pointerSrc;
      return getDestVal()==4;
   }

   override public function getGeneration()
   {
      return super.getGeneration() + 1;
   }
   #else
   override public function getGeneration()
   {
      return super.getGeneration() + 1;
   }
   #end

   override public function whoStartedYou() : String  return super.whoStartedYou();

   // override IHostInteface 
   override public function whoOverridesYou() return "ClientExtends";

   // new IClientInterface
   public function uniqueClientFunc() return "uniqueClientFunc";

   // IClientHostInterface
   public function whoAreYou() return "ClientExtends";

   public function getOne() return 1;

   public function getTwo() return 2;

   public function getThree() return 3;

   override public function update() return "ClientExtends update";
}


