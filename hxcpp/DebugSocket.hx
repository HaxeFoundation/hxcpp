package hxcpp;

import haxe.Stack;
import hxcpp.DebugBase;
import cpp.net.Socket;
import cpp.vm.Thread;

class OutputMessage
{
   public function new(inType:Int, inValue:Dynamic)
   {
      type = inType;
      value = inValue;
   }
   public static inline var RESULT = 0;
   public static inline var STATUS = 1;
   public static inline var OUTPUT = 2;

   public var type:Int;
   public var value:Dynamic;
}

class DebugSocket extends DebugStdio
{
   var socket:Socket;
   var host:String;
   var port:Int;

   var outputThread:Thread;
   var socketOut:haxe.io.Output;

   public function new(inHost:String, inPort:Int, inCreateStopped:Bool=false)
   {
      host = inHost;
      port = inPort;
      super(inCreateStopped);
   }

   override function init() : Bool
   {
      try
      {
         socket = new Socket();
         trace("Connect " + host + ":" + port + "...");
         try
         {
            socket.connect( new cpp.net.Host(host), port );
         } catch (e:Dynamic)
         {
             trace("Could not CONNECT!");
             socket = null;
         }

         if (socket!=null)
         {
            trace("connected.");
            input = socket.input;
            socketOut = socket.output;
         }
         else
         {
            trace("debugging failed.");
            return false;
         }
      }
      catch(e:Dynamic)
      {
         if (socket!=null)
            socket.close();
         socket = null;
         trace("Socket error:" + e);
         return false;
      }

      outputThread = Thread.create(outputLoop);
      return true;
   }

   override function getNextCommand() : String
   {
      if (input==null)
         return "bye";
      try
      {
         return input.readLine();
      }
      catch(e:Dynamic)
      {
         trace("getNextCommand - socket closed");
         input.close();
         input = null;
      }
      return "bye";
   }

   override function onResult(inString:String)
   {
      if (stillDebugging)
         outputThread.sendMessage(new OutputMessage(OutputMessage.RESULT,inString) );
   }

   override function sendOutput(inString:String)
   {
      if (stillDebugging)
         outputThread.sendMessage(new OutputMessage(OutputMessage.OUTPUT,inString) );
   }

   override function sendStatus(inString:String)
   {
      if (stillDebugging)
         outputThread.sendMessage(new OutputMessage(OutputMessage.STATUS,inString) );
   }

   function outputLoop()
   {
      while(stillDebugging)
      {
         var message:OutputMessage = Thread.readMessage(true);
         socketOut.writeByte(message.type);
         var value:String = message.value;
         var len = value.length;
         socketOut.writeInt31(len);
         socketOut.writeString(value);
      }
      socketOut.close();
   }

}


