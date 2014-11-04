package hxcpp;

class NekoInit
{
   public static function nekoInit(inModuleName:String) : Bool
   {
      var init = neko.Lib.load(inModuleName, "neko_init", 5);

      if (init != null) 
      {
         init( function(s) return new String(s),
               function(len:Int) { var r = []; if (len > 0) r[len - 1] = null; return r; },
               null,
               true,
               false);
         return true;

      }
      return false;
   }
}
