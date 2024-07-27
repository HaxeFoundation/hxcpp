
class TestMain
{
   public static function main()
   {
      utest.UTest.run([ new TestCffi(), new TestPrime() ]);
   }
}


