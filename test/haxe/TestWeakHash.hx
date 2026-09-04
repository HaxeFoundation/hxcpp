import utest.Test;
import utest.Assert;
import haxe.ds.WeakMap;

class WeakObjectData
{
   public var id:Int;
   public function new(inId:Int) id = inId;
   public function toString() return "Data " + id;
}

class WeakValueData
{
   public var key:WeakObjectData;
   public function new(inKey:WeakObjectData) key = inKey;
}

class TestWeakHash extends Test
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
      Assert.isTrue(oddFound<=2, "Too many odd values retained " + oddFound);
      Assert.isTrue(valid>=expect && valid<expect+2, "WeakHash invalid range "+ expect + "..." + valid + "..." + (expect+2));
   }

   public function test()
   {
      var map : WeakMap<WeakObjectData,Int> = null;

      final sema = new sys.thread.Semaphore(0);
      sys.thread.Thread.create(() -> {
         map = createMap(1000);
         sema.release();
      });
      sema.acquire();

      // Give the thread enough time to exit and unregister itself from the GC
      Sys.sleep(1);

      cpp.vm.Gc.run(true);

      final sema = new sys.thread.Semaphore(0);
      sys.thread.Thread.create(() -> {
         checkMap(map,500);
         sema.release();
      });
      sema.acquire();

      // Give the thread enough time to exit and unregister itself from the GC
      Sys.sleep(1);

      final sema = new sys.thread.Semaphore(0);
      sys.thread.Thread.create(() -> {
         retained = [];
         sema.release();
      });
      sema.acquire();

      // Give the thread enough time to exit and unregister itself from the GC
      Sys.sleep(1);

      cpp.vm.Gc.run(true);

      final sema = new sys.thread.Semaphore(0);
      sys.thread.Thread.create(() -> {
         checkMap(map,0);
         sema.release();
      });
      sema.acquire();

      // Give the thread enough time to exit and unregister itself from the GC
      Sys.sleep(1);
      
      Assert.pass();
   }

   public function testDeadKeyReleasesValueCycle()
   {
      var result:{
         map:WeakMap<WeakObjectData,WeakValueData>,
         key:cpp.vm.WeakRef<WeakObjectData>,
         value:cpp.vm.WeakRef<WeakValueData>
      } = null;

      final sema = new sys.thread.Semaphore(0);
      sys.thread.Thread.create(() -> {
         var map = new WeakMap<WeakObjectData,WeakValueData>();
         var key = new WeakObjectData(1);
         var value = new WeakValueData(key);
         map.set(key,value);
         result = {
            map: map,
            key: new cpp.vm.WeakRef(key),
            value: new cpp.vm.WeakRef(value)
         };
         sema.release();
      });
      sema.acquire();

      // Ensure no conservative reference remains on the terminated thread stack.
      Sys.sleep(1);
      cpp.vm.Gc.run(true);

      Assert.isNull(result.key.get());
      Assert.isNull(result.value.get());
      Assert.isFalse(result.map.keys().hasNext());
   }

   public function testLiveKeyRetainsValue()
   {
      var map:WeakMap<WeakObjectData,WeakValueData> = null;
      var value:cpp.vm.WeakRef<WeakValueData> = null;

      final sema = new sys.thread.Semaphore(0);
      sys.thread.Thread.create(() -> {
         map = new WeakMap<WeakObjectData,WeakValueData>();
         var key = new WeakObjectData(2);
         var mapValue = new WeakValueData(key);
         retained = [key];
         map.set(key,mapValue);
         value = new cpp.vm.WeakRef(mapValue);
         sema.release();
      });
      sema.acquire();

      Sys.sleep(1);
      cpp.vm.Gc.run(true);

      Assert.notNull(value.get());
      Assert.notNull(map.get(retained[0]));
   }

   public function testEphemeronsReachFixedPoint()
   {
      var maps:{
         upstream:WeakMap<WeakObjectData,WeakObjectData>,
         downstream:WeakMap<WeakObjectData,WeakObjectData>
      } = null;
      var value:cpp.vm.WeakRef<WeakObjectData> = null;

      final sema = new sys.thread.Semaphore(0);
      sys.thread.Thread.create(() -> {
         // Register downstream first so it is visited before upstream. A
         // single pass would skip it before upstream makes its key reachable.
         var downstream = new WeakMap<WeakObjectData,WeakObjectData>();
         var upstream = new WeakMap<WeakObjectData,WeakObjectData>();
         var rootKey = new WeakObjectData(3);
         var linkKey = new WeakObjectData(4);
         var finalValue = new WeakObjectData(5);
         downstream.set(linkKey,finalValue);
         upstream.set(rootKey,linkKey);
         retained = [rootKey];
         maps = {upstream: upstream, downstream: downstream};
         value = new cpp.vm.WeakRef(finalValue);
         sema.release();
      });
      sema.acquire();

      Sys.sleep(1);
      cpp.vm.Gc.run(true);

      var linkKey = maps.upstream.get(retained[0]);
      Assert.notNull(linkKey);
      Assert.notNull(maps.downstream.get(linkKey));
      Assert.notNull(value.get());
   }

}
