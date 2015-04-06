#if macro
import haxe.macro.Expr;
#end


class Loader
{
   #if cpp
   public static function __init__()
   {
      cpp.Lib.pushDllSearchPath( "project/ndll/" + cpp.Lib.getBinDirectory() );
   }
   #end

   public static inline macro function load(inName2:Expr, inSig:Expr)
   {
      return macro cpp.Prime.load("prime", $inName2, $inSig, false);
   }
}

