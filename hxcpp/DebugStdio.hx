package hxcpp;

import haxe.Stack;
import cpp.vm.Debugger;
import hxcpp.DebugBase;
import Type;


class DebugStdio extends DebugBase
{
   var input:haxe.io.Input;

   public function new(inCreateStopped:Bool=false)
   {
      super(inCreateStopped);
   }

   override function init():Bool
   {
      input = Sys.stdin();
      return true;
   }

   override function onCloseInput()
   {
      if (input!=null)
         input.close();
   }


   override function getNextCommand() : String
   {
      Sys.print("debug>");
      return input.readLine();
   }


   function sendOutput(inString:String)
   {
      Sys.println(inString);
   }

   function sendStatus(inString:String)
   {
      sendOutput(inString);
   }

   override function sendString(inString:String)
   {
      sendOutput(inString);
   }

   override function onRunning()
   {
      sendStatus("running");
   }


   override function onStopped()
   {
      sendStatus("stopped.");
   }

   override function showWhere()
   {
      var idx = 0;
      for(item in stack)
      {
         idx++;
         sendOutput((idx==frame ? "*" : " ") + idx + ":" + item);
      }
   }

   override function showFiles()
   {
       if (files!=null)
         for(idx in 0...files.length)
            sendOutput("file " + idx + " : " + files[idx] );
   }

   override function showBreakpoints()
   {
      var bps = Debugger.getBreakpoints();
       if (bps!=null)
         for(idx in 0...bps.length)
            sendOutput("breakpoint " + idx + " : " + bps[idx] );
   }

   function printRec(inPrefix:String, result:Dynamic, inLevel:Int)
   {
      if (result==null)
         sendOutput(inPrefix + "(null)");
      else if ( untyped result.__IsArray() )
      {
         var len = result.length;
         var lim = len<arrayLimit ? len : arrayLimit;
         if (lim==0)
            sendOutput(inPrefix + "(empty)");
         else if (inLevel>0)
            sendOutput(inPrefix + len + " elements");
         else
         {
            for(i in 0...lim)
               printRec(inPrefix + i + "] ", result[i], inLevel+1);
            if (lim<len)
               sendOutput(inPrefix + "..." + len + "elements");
         }
      }
      else switch Type.typeof(result)
      {
         case TObject:
            sendObject(inPrefix,result,inLevel, "{...}");
         case TClass(c):
            if (c==String)
               sendOutput(inPrefix+result);
            else
               sendObject(inPrefix,result,inLevel,c+"");
         default:
            sendOutput(inPrefix + result);
      }
   }

   function sendObject(inPrefix:String,result:Dynamic,inLevel:Int, inClass:String)
   {
      if (inLevel>0)
         sendOutput(inPrefix + inClass);
      else
      {
         var fields = Reflect.fields(result);
         if (files.length==0)
            sendOutput(inPrefix + "{}");
         else
            for(field in fields)
            {
               printRec(inPrefix + field + "=",  Reflect.getProperty(result,field), inLevel+1 );
            }
      }
   }


   override function onPrint(result:Dynamic)
   {
      printRec("",result,0);
   }

   override function onResult(inResult:String)
   {
      sendOutput(inResult);
   }


}


