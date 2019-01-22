
class Entry
{
   public var name:String;
   public var entry:Float;
   public var total:Float;
   public var running:Bool;
   public var children:Array<Entry>;
   public var current:Entry;

   public function new(inName:String)
   {
      name = inName;
      total = 0.0;
      children = [];
      entry = haxe.Timer.stamp();
      running = true;
      current = null;
   }

   public function start()
   {
      if (running)
         trace("===== Restarted " + name);
      entry = haxe.Timer.stamp();
      running = true;
   }

   public function stop()
   {
      if (running)
      {
         total += haxe.Timer.stamp() - entry;
         running = false;
      }
   }

   public function find(inName:String)
   {
      for(c in children)
         if (c.name==inName)
         {
            current = c;
            current.start();
            return current;
         }
      var result = new Entry(inName);
      children.push(result);
      current = result;
      return result;
   }


   static function timeString(t:Float)
   {
      return Std.int(t*1000.0) + "ms";
   }

   public function dump(indent = "")
   {
      if (running)
         trace("========== running?");
      Sys.println(indent + name + " : " + timeString(total) );
      for(c in children)
         c.dump( indent + "  " );
   }
}




class Profile
{
   static var valid = false;
   static var t0:Float;
   static var root:Entry;
   static var current:Entry;
   static var currentParent:Entry;
   static var stack:Array<Entry>;


   public static function start()
   {
      t0 = now();
      root = new Entry("Program");
      currentParent = root;
      current = null;
      stack = [];
   }

   public static function enable()
   {
      valid = true;
      Tools.addOnExitHook( dump );
   }

   public static function setEntry(inName:String)
   {
      if (!valid) return;

      if (current!=null)
         current.stop();
      current = currentParent.find(inName);
   }

   public static function push(inName:String)
   {
      if (!valid) return;

      stack.push(currentParent);
      currentParent = current;
      current = currentParent.find(inName);
   }
   public static function pop()
   {
      if (!valid) return;

      current.stop();
      currentParent = stack.pop();
      current = currentParent.current;
   }


   static function dump(inExitCode:Int)
   {
      if (inExitCode==0)
      {
         if (current!=null)
            current.stop();
         if (currentParent!=null)
            currentParent.stop();
         for(s in stack)
            s.stop();
         root.dump();
      }
   }

   inline static function now() return haxe.Timer.stamp();
}

