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
      if (c.whoStartedYou()!="HostBase")
      {
         Common.status = "Bad class fallthrough - got " + c.whoStartedYou();
         return;
      }
      if (c.whoOverridesYou()!="ClientExtends")
      {
         Common.status = "Bad class override - got " + c.whoOverridesYou();
         return;
      }
      var hostInterface:IHostInterface = c;
      if (hostInterface.whoStartedYou()!="HostBase")
      {
         Common.status = "Bad interface fallthrough";
         return;
      }
      if (hostInterface.whoOverridesYou()!="ClientExtends")
      {
         Common.status = "Bad interface override";
         return;
      }
      if (hostInterface.hostImplOnly(1,"two",3)!="1two3")
      {
         Common.status = "Bad hostImplOnly implementation";
         return;
      }


      var clientInterface:IClientInterface = c;
      if (clientInterface.whoStartedYou()!="HostBase")
      {
         Common.status = "Bad client interface fallthrough";
         return;
      }
      if (clientInterface.uniqueClientFunc()!="uniqueClientFunc")
      {
         Common.status = "Bad new client interface call";
         return;
      }

      if (clientInterface.whoOverridesYou()!="ClientExtends")
      {
         Common.status = "Bad client interface override";
         return;
      }

      var clientHostInterface:IClientHostInterface = c;
      if (clientHostInterface.whoStartedYou()!="HostBase")
      {
         Common.status = "Bad client interface fallthrough";
         return;
      }
      if (clientHostInterface.whoOverridesYou()!="ClientExtends")
      {
         Common.status = "Bad client interface override";
         return;
      }
      if (clientHostInterface.whoAreYou()!="ClientExtends")
      {
         Common.status = "Bad client/host interface";
         return;
      }

      var c:ClientIHostImpl = new ClientIHostImpl();
      if (c.hostImplOnly(0,null,0)!="client" || c.whoStartedYou()!="client" || c.whoOverridesYou()!="client")
      {
         Common.status = "Trouble implementing host interface";
         return;
      }


      Common.clientImplementation = new ClientOne();
      Common.status = "ok";
   }
}
