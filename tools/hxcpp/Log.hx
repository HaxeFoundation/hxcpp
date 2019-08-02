import haxe.io.Bytes;
import sys.io.Process;

#if haxe4
import sys.thread.Mutex;
#elseif neko
import neko.vm.Mutex;
#else
import cpp.vm.Mutex;
#end

#if neko
import neko.Lib;
#else
import cpp.Lib;
#end

class Log
{
   public static var mute:Bool= false;
   public static var quiet:Bool = false;
   public static var verbose:Bool = false;

   public  static var colorSupported:Null<Bool> = null;
   private static var sentWarnings = new Map<String,Bool>();

   public static var printMutex:Mutex;

   public static inline var RED = "\x1b[31m";
   public static inline var YELLOW = "\x1b[33m";
   public static inline var WHITE = "\x1b[37m";
   public static inline var NORMAL = "\x1b[0m";
   public static inline var BOLD = "\x1b[1m";
   public static inline var ITALIC = "\x1b[3m";

   public static function initMultiThreaded()
   {
     if (printMutex==null)
        printMutex = new Mutex();
   }

   public static function e(message:String):Void
   {
      error(message);
   }
   public static function error(message:String, verboseMessage:String = "", e:Dynamic = null, terminate:Bool = true):Void
   {
      var output;
      if (verbose && verboseMessage != "")
      {
         output = "\x1b[31;1mError:\x1b[0m\x1b[1m " + verboseMessage + "\x1b[0m\n";
      }
      else
      {
         if (message=="")
            output = "\x1b[31;1mError\x1b[0m\n";
         else
            output = "\x1b[31;1mError:\x1b[0m \x1b[1m" + message + "\x1b[0m\n";
      }
      if (printMutex!=null)
         printMutex.acquire();
      Sys.stderr().write(Bytes.ofString(stripColor(output)));
      if (printMutex!=null)
         printMutex.release();

      if ((verbose || !terminate) && e != null)
         Lib.rethrow(e);

      if (terminate)
         Tools.exit(1);
   }

   public static function info(message:String, verboseMessage:String = ""):Void
   {
      if (!mute)
      {
         if (printMutex!=null)
            printMutex.acquire();
         if (verbose && verboseMessage != "")
         {
            println(verboseMessage);
         }
         else if (message != "")
         {
            println(message);
         }
         if (printMutex!=null)
            printMutex.release();
      }
   }
   inline public static function v(verboseMessage:String):Void
   {
      Log.info("",verboseMessage);
   }

   public static function lock():Void
   {
      if (printMutex!=null)
        printMutex.acquire();
   }

   public static function unlock():Void
   {
      if (printMutex!=null)
        printMutex.release();
   }


   public static function print(message:String):Void
   {
      if (printMutex!=null)
        printMutex.acquire();
      Sys.print(stripColor(message));
         if (printMutex!=null)
            printMutex.release();
   }

   public static function println(message:String):Void
   {
      if (printMutex!=null)
        printMutex.acquire();
      Sys.println(stripColor(message));
      if (printMutex!=null)
         printMutex.release();
   }

   private static function stripColor(output:String):String
   {
      if (colorSupported == null)
      {
         if (!BuildTool.isWindows)
         {
            var result = -1;
            try
            {
               var process = new Process ("tput", [ "colors" ]);
               result = process.exitCode ();
               process.close ();
            }
            catch (e:Dynamic) {};

            colorSupported = (result == 0);
         }
         else
         {
            colorSupported = (Sys.getEnv("TERM") == "xterm" || Sys.getEnv("ANSICON") != null);
         }
      }

      if (colorSupported)
      {
         return output;
      }
      else
      {
         var colorCodes:EReg = ~/\x1b\[[^m]+m/g;
         return colorCodes.replace(output, "");
      }
   }

   public static function warn(message:String, verboseMessage:String = "", allowRepeat:Bool = false):Void
   {
      if (!mute)
      {
         var output = "";
         if (verbose && verboseMessage != "")
         {
            output = "\x1b[33;1mWarning:\x1b[0m \x1b[1m" + verboseMessage + "\x1b[0m";
         }
         else if (message != "")
         {
            output = "\x1b[33;1mWarning:\x1b[0m \x1b[1m" + message + "\x1b[0m";
         }

         if (!allowRepeat && sentWarnings.exists (output))
         {
            return;
         }

         sentWarnings.set(output, true);

         if (printMutex!=null)
            printMutex.acquire();
         println(output);
         if (printMutex!=null)
            printMutex.release();
      }
   }
}
