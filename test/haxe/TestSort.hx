class SortData
{
   public var value:Int;
   public var id:Int;
   static var ids = 0;

   public function new()
   {
      value = Std.int(Math.random()*500);
      id = ids++;
   }
}


class TestSort extends haxe.unit.TestCase
{
   public function testObjects()
   {
      var tests = new Array<SortData>();
      for(i in 0...100000)
         tests.push( new SortData() );

      var sorted = tests.copy();
      sorted.sort( function(a,b) return a.value - b.value );

      for(i in 1...sorted.length)
      {
         if (sorted[i].value < sorted[i-1].value)
            throw "Index out of order";
         if (sorted[i].value == sorted[i-1].value &&
               sorted[i].id <= sorted[i-1].id )
            throw "Not stable sort";
      }

      var sorted = tests.copy();
      var compares = 0;
      sorted.sort( function(a,b) {
          // Churn some GC
          var array = new Array<Int>();
          compares++;
          array.push(a.value);
          array.push(b.value);
          return array[0] < array[1] ? 1 : array[0] > array[1] ? -1 : 0;
       });

      //Sys.println("\nCompares per log elements:" + (compares/(tests.length*Math.log(tests.length))));

      for(i in 1...sorted.length)
      {
         if (sorted[i].value > sorted[i-1].value)
            throw "Index out of order";
         if (sorted[i].value == sorted[i-1].value &&
               sorted[i].id <= sorted[i-1].id )
            throw "Not stable sort";
      }

      assertTrue(true);
   }
}

