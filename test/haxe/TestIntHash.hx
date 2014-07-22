class TestIntHash extends haxe.unit.TestCase
{
   function spamAlot()
   {
      var values = new Array< Null<Int> >();
      values[0] = null;
      var h = new Map<Int,Int>();

      for(i in 0...2000)
      {
         for(j in 0...2000)
         {
            var idx = Std.int(Math.random()*2999);
            if (h.get(idx)!=values[idx])
               throw "Bad value";
            if ( (i % 4)== 1 || Math.random()>0.5 )
            {
               if (h.remove(idx) != (values[idx]!=null))
                  throw "Error in remove!";
               values[idx] = null;
            }
            else
            {
               h.set(idx,j);
               values[idx] = j;
            }
         }
         var keys = h.keys();
         var keyed = new Array<Bool>();
         for(i in 0...values.length)
            keyed[i] = false;
         for( key in h.keys())
            keyed[ key ] = true;
         for(i in 0...values.length)
            if (keyed[i]!=(values[i]!=null))
              throw "Bad value";

         var valued = new Array<Int>();
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
      catch(e:String)
      {
         err = e;
      }
      assertTrue(err=="");
   }

}
