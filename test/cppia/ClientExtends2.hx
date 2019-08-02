class ClientExtends2 extends ClientExtends
{
   public function new()
   {
      super();
   }

   override public function getGeneration()
   {
      return super.getGeneration()+1;
   }

   public function getFour() return 4;

   public function testFour() : Bool
   {
      return getFour()==4;
   }

   override public function getOne() return 111;

   override public function update() return "ClientExtends2 update";

}

