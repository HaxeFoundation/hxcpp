import utest.Assert;
import cases.client.*;

class ClientOne implements pack.HostInterface
{
   public function new() { }
   public function getOne() : Int return 1;
   public function getOneString() : String return "1";
}

class ClientFoo implements IFoo {

   public function new() {}

   public function baz():String {
       return 'foo';
   }
}

class Client {
   public static function setupCommon() {
      // client-side setup for host TestCommon
      Common.clientImplementation = new ClientOne();

      Common.callback = () -> Common.callbackSet = 2;
      Common.instance.dynamicMethod = () -> Common.callbackSet = 3;
   }

   public static function useHostImplementation() {
      // invoked by host TestCommon, so Asserts are in-context
      Assert.equals(1, Common.hostImplementation.getOne());
      Assert.equals("1", Common.hostImplementation.getOneString());
   }

   public static function main() {
      Common.CLIENT_CASES = [
         new TestClient(),
         new LocalFunctionExceptions(),
         new ReturnExpressions()
      ];
   }
}
