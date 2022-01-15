package gc;

class TestBigStack
{
   var refs:Array<TestBigStack>;
   var self:TestBigStack;

   public function new()
   {
      refs = [];
      self = this;
   }

   function check()
   {
      if (self!=this)
         throw("Bad self reference");
   }


   function checkRec()
   {
      if (self!=this)
         throw("Bad self reference");
      for(r in refs)
        r.checkRec();
   }

   function runRec(depth:Int)
   {
      var d = depth-1;
      if (d==0)
         return this;
      var b0 = new TestBigStack().runRec(d);
      var b1 = new TestBigStack().runRec(d);
      var b2 = new TestBigStack().runRec(d);
      var b3 = new TestBigStack().runRec(d);
      var b4 = new TestBigStack().runRec(d);
      var b5 = new TestBigStack().runRec(d);
      var b6 = new TestBigStack().runRec(d);
      var b7 = new TestBigStack().runRec(d);
      var b8 = new TestBigStack().runRec(d);
      var b9 = new TestBigStack().runRec(d);
      var b10 = new TestBigStack().runRec(d);
      var b11 = new TestBigStack().runRec(d);
      var b12 = new TestBigStack().runRec(d);
      var b13 = new TestBigStack().runRec(d);
      var b14 = new TestBigStack().runRec(d);
      var b15 = new TestBigStack().runRec(d);
      var b16 = new TestBigStack().runRec(d);
      var b17 = new TestBigStack().runRec(d);
      var b18 = new TestBigStack().runRec(d);
      var b19 = new TestBigStack().runRec(d);
      refs.push(b0);
      b0.check();
      b1.check();
      b2.check();
      b3.check();
      b4.check();
      b5.check();
      b6.check();
      b7.check();
      b8.check();
      b9.check();
      b10.check();
      b11.check();
      b12.check();
      b13.check();
      b14.check();
      b15.check();
      b16.check();
      b17.check();
      b18.check();
      b19.check();
      return this;
   }

   function run(passes:Int, depth:Int)
   {
      for(p in 0...passes)
      {
         //if ( (p%1000)==0 )
         //   Sys.println('Pass $p...');
         refs = [];
         runRec(depth);
         checkRec();
      }
   }

   public static function test() : Bool
   {
      try {
         var b = new TestBigStack();
         b.run(5000,5);
         return true;
      }
      catch(e:Dynamic)
      {
         return false;
      }
   }
}
