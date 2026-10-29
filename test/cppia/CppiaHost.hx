import utest.UTest;
import utest.Test;
import cpp.cppia.Host;

class CppiaHost extends Test
{
   public static function main() {
      // load cppia client
      Host.main();

      UTest.run(Common.CLIENT_CASES.concat([ new cases.TestCommon() ]));
   }
}
