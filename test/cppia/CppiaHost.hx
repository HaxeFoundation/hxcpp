import cpp.cppia.Host;

import HostBase;

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
      }
   }
}
