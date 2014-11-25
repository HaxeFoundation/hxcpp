class ObjectData
{
   public var id:Int;
   public function new(inId:Int) id = inId;
}


class TestObjectHash extends haxe.unit.TestCase
{
   function spamAlot()
   {
      var values = new Array< Null<Int> >();
      values[0] = null;
      var h = new Map<ObjectData,Int>();
      var idxToKey = new Array<ObjectData>();
      for(idx in 0...3000)
         idxToKey[idx] = new ObjectData(idx);

      for(i in 0...2000)
      {
         for(j in 0...2000)
         {
            var idxInt = Std.int(Math.random()*2999);
            var idx = idxToKey[idxInt];
            if (h.get(idx)!=values[idxInt])
            {
               throw "Bad value";
            }
            if ( (i % 4)== 1 || Math.random()>0.5 )
            {
               if (h.remove(idx) != (values[idxInt]!=null))
               {
                  trace("Bad remove");
                  throw "Error in remove!";
               }
               values[idxInt] = null;
            }
            else
            {
               h.set(idx,j);
               values[idxInt] = j;
            }
         }
         var keys = h.keys();
         var keyed = new Array<Bool>();
         for(i in 0...values.length)
            keyed[i] = false;
         for( key in h.keys())
         {
            var idxInt = key.id;
            keyed[ idxInt ] = true;
         }
         for(i in 0...values.length)
            if (keyed[i]!=(values[i]!=null))
               throw "Bad value";

         var valued = new Array<Int>();
         #if neko
         valued[3000]=0;
         for(i in 0...3000)
            valued[i] = 0;
         #end
         for( val in h.iterator())
            valued[val]++;

         for(val in values)
            if (val!=null)
            {
               if (valued[val]<1)
                  throw "Not valued!";
               valued[val]--;
            }
      }
   }

   public function test()
   {
      var err = "";
      try
      {
         spamAlot();
      }
      catch(e:Dynamic)
      {
         trace(e);
         err = e;
      }
      assertTrue(err=="");
   }

}
