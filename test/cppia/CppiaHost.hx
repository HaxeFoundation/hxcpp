import utest.UTest;
import utest.Test;

class CppiaHost extends Test
{
   public static function main() {
      UTest.run([ new cases.TestCommon() ]);
   }
}