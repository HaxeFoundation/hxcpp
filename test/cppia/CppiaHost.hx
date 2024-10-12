import cpp.cppia.Host;

import HostBase;
import HostExtendedRoot;

class HostOne implements pack.HostInterface
{
   public static var called = 0;

   public function new()
   {
   }

   public function getOne() : Int
   {
      called ++;
      return 1;
   }
   public function getOneString() : String
   {
     called++;
     return "1";
   }
}

class CppiaHost
{

   public static function main()
   {
      Common.hostImplementation = new HostOne();

      Common.callback = () -> Common.callbackSet = 1;

      /*
      if (new HostExtends().getYou().extendOnly != 1)
      {
         Sys.println("extend-overide type failed");
         Sys.exit(-1);
      }
      */

      Host.main();
      Sys.println("TestStatus: " + Common.status );
      if (Common.status!="ok")
      {
         Sys.println("failed");
         Sys.exit(-1);
      }
      else
      {
         if (HostOne.called!=2)
         {
            Sys.println("No client implementation call - failed");
            Sys.exit(-1);
         }

         if (Common.clientImplementation==null)
         {
            Sys.println("No client implementation - failed");
            Sys.exit(-1);
         }
         if (Common.clientImplementation.getOne()!=1)
         {
            Sys.println("Bad client Int implementation - failed");
            Sys.exit(-1);
         }
         if (Common.clientImplementation.getOneString()!="1")
         {
            Sys.println("Bad client String implementation - failed");
            Sys.exit(-1);
         }

         var hostBase:HostBase = Type.createInstance(Type.resolveClass("ClientExtends2"),[]);

         if (!hostBase.testUpdateOverride())
         {
            Sys.println("Bad update override");
            Sys.exit(-1);
         }

         Common.callback();
         if (Common.callbackSet!=2)
         {
            Sys.println("Bad cppia closure");
            Sys.exit(-1);
         }

         #if (haxe >= version("4.3.6"))
         if (Common.clientRoot == null) {
            Sys.println("null client root class");
            Sys.exit(-1);
         }
         switch Common.clientRoot.values {
            case [ 0, 1, 2 ]:
               //
            case _:
               Sys.println("Unexpected items in array");
               Sys.exit(-1);   
         }
         #end
      }
   }
}

