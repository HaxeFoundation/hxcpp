import haxe.ds.WeakMap;

class WeakObjectData
{
   public var id:Int;
   public function new(inId:Int) id = inId;
   public function toString() return "Data " + id;
}


class TestWeakHash extends haxe.unit.TestCase
{
   var retained:Array<WeakObjectData>;

   function createMap(inCount:Int)
   {
      retained = [];
      var map = new WeakMap<WeakObjectData,Int>();
      for(i in 0...inCount)
      {
         var obj = new WeakObjectData(i);
         if ( (i&1)==0 )
            retained.push(obj);
         map.set(obj,i);
      }
      return map;
   }
   function createMapDeep(inDepth:Int, inCount:Int)
   {
      if (inDepth<1)
         return createMap(inCount);

      return createMapDeep(inDepth-1, inCount);
   }

   function checkMap(map:WeakMap<WeakObjectData,Int>, expect:Int)
   {
      var valid = 0;
      var oddFound = 0;
      for(k in map.keys())
      {
         if( (k.id&1)!= 0)
         {
            oddFound ++;
            //throw "Odd retained " + k.id;
         }
         else
            valid++;
      }
      // There may be one or two values lurking on the stack, which is conservatively marked
      if (oddFound>2)
         trace("Too many odd values retained " + oddFound);
      if (!(valid>=expect && valid<expect+2))
         trace("WeakHash invalid range "+ expect + "..." + valid + "..." + (expect+2) );
      assertTrue(valid>=expect && valid<expect+2);
   }
   function deepCheckMap(inDepth:Int, map:WeakMap<WeakObjectData,Int>, expect:Int)
   {
      if (inDepth<1)
         checkMap(map,expect);
      else
         deepCheckMap(inDepth-1, map, expect);
   }

   function deepClearRetained(inRecurse:Int)
   {
      if (inRecurse>0)
         deepClearRetained(inRecurse-1);
      else
         retained = [];
   }

   public function test()
   {
      var err = "";
      try
      {
         var map = createMapDeep(20,1000);
         cpp.vm.Gc.run(true);
         deepCheckMap(10,map,500);
         deepClearRetained(10);
         cpp.vm.Gc.run(true);
         checkMap(map,0);
      }
      catch(e:String)
      {
         trace(e);
         err = e;
      }
      assertTrue(err=="");
   }

}
