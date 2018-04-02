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
}

