class ClientOne implements pack.HostInterface
{
   public function new() { }
   public function getOne() : Int return 1;
   public function getOneString() : String return "1";
}

class Client
{
   public static var clientBool0 = true;
   public static var clientBool1 = false;
   public static var clientBool2 = true;
   public static var clientBool3 = false;

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
      if (!c.testPointers())
      {
         Common.status = "Could not move native pointers";
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

      if (!c.testOne())
      {
         Common.status = "Bad ClientExtends getOne";
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

      var c:ClientExtends = new ClientExtends2();
      if (c.getGeneration()!=2)
      {
         Common.status = "Error calling cppia super function";
         return;
      }

      var c = new ClientExtends2();
      if (c.testOne())
      {
         Common.status = "ClientExtends2 getOne should fail";
         return;
      }

      if (!c.testOneExtended())
      {
         Common.status = "ClientExtends2 testOneExtended failed";
         return;
      }

      if (!c.testFour())
      {
         Common.status = "ClientExtends2 testFour error";
         return;
      }



      var hostBools = HostBase.hostBool0 + "/" + HostBase.hostBool1+ "/" + HostBase.hostBool2+ "/" + HostBase.hostBool3;
      var clientBools = clientBool0 + "/" + clientBool1+ "/" + clientBool2+ "/" + clientBool3;
      if (hostBools!=clientBools)
      {
         Common.status = "Error in bool representation:" + hostBools + "!=" + clientBools;
         return;
      }

      Common.clientImplementation = new ClientOne();
      Common.status = "ok";
   }
}
