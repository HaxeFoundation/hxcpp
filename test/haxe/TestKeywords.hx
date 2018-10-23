private enum GnarlyEnum
{
   e0;
   GnarlyEnum;
   narlyEnum;
   Dynamic;
   getFixed(i:Int);
   getInt;
   init(i:Int);
   String;
   index(i:Int);
   const;
   super(i:Int);
   tag(i:Int);
   getTag(i:Int);
   getObject(i:Int);
}

class TestKeywords extends haxe.unit.TestCase
{
   public function new() super();

   //public function getGnarly() { return GnarlyEnum.super(1); }
   public function getGnarly() { return Dynamic; }

   public function testEnum()
   {
      var count = 
      switch( getGnarly() )
      {
         case e0: 1;
         //case GnarlyEnum: 1;
         case narlyEnum: 1;
         case Dynamic: 3;
         case getFixed(i): 1;
         case getInt: 1;
         case init(i): 1;
         case String: 1;
         case index(i): 1;
         case const: 1;
         //case GnarlyEnum.super(i): 2;
         case tag(i): 1;
         case getTag(i): 1;
         case getObject(i): 1;
         default: 0;
      }
      assertTrue(count==3);
   }
}
