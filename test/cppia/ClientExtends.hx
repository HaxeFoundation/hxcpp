class ClientExtends extends HostBase
{
   public function new()
   {
      super();
   }

   public function ok():Bool
   {
      return getVal()==1.25;
   }
}
