class ClientOne implements pack.HostInterface
{
   public function new() { }
   public function getOne() : Int return 1;
   public function getOneString() : String return "1";
}

class Client
{
   public static function main()
   {
      Common.status = "running";
      if (Common.hostImplementation.getOne()!=1)
      {
         Common.status = "Bad call to getOne";
         return;
      }
      if (Common.hostImplementation.getOneString()!="1")
      {
         Common.status = "Bad call to getOneString";
         return;
      }

      var c = new ClientExtends();
      if (!c.ok())
      {
         Common.status = "Bad client extension";
         return;
      }

      Common.clientImplementation = new ClientOne();
      Common.status = "ok";
   }
}
