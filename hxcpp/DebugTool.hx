package hxcpp;

#if neko
import sys.net.Socket;
import sys.net.Host;
import neko.vm.Thread;
#else
import cpp.net.Socket;
import cpp.net.Host;
#end

import haxe.Timer;


class DebugTool
{
   var dbg:Socket;
   var server:Socket;
   var readBuffer:haxe.io.Bytes;
   var fromApp:haxe.io.Input;
   var toApp:haxe.io.Output;
   var port:Int;
   var mainThread:Thread;
   var readThread:Thread;
   var going:Bool;

   static var RESULT = 0;
   static var STATUS = 1;
   static var OUTPUT = 2;

  function new(inHost:String, inPort:Int)
  {
     trace(inHost + ":" + inPort);
     try
     {
        server = new Socket();
        var host = new Host(inHost);
        Sys.println("Waiting for connection on " + host + ":" + inPort);
        server.bind(host,inPort);
        server.listen(1);
        port = inPort;

        // Wait for connection....
        dbg = server.accept();

        trace("Got connection:" + ok());
        if (ok())
        {
           fromApp = dbg.input;
           toApp = dbg.output;
        }
     }
     catch(e:Dynamic)
     {
        Sys.println("Could not create dbg server on " + inHost + ":" + inPort);
     }

  }

  public function ok() { return dbg!=null; }

  function readString()
  {
     var len = fromApp.readInt31();
     return fromApp.readString(len);
  }


  function readLoop()
  {
     try
     {
        while(going)
        {
           var code = fromApp.readByte();
           if (code>OUTPUT)
           {
              Sys.println("Unknown message code : " + code);
              mainThread.sendMessage("bye");
              return;
           }
           var value = readString();
           switch(code)
           {
              case OUTPUT: Sys.println(value);
              case STATUS: Sys.println(value);
              case RESULT: mainThread.sendMessage(value);
           }
        }
     }
     catch (e:Dynamic)
     {
        going = false;
        Sys.println("Connection terminated:" + e);
        mainThread.sendMessage("bye");
     }
     trace("readLoop done");
  }


  function mainLoop( )
  {
     trace("mainLoop");
     going = true;

     mainThread = Thread.current();
     readThread = Thread.create(readLoop);
     var stdin = Sys.stdin();

     while(going)
     {
        Sys.print(" => ");
        var command = stdin.readLine();

        if (command!="")
        {
           toApp.writeString( command + "\n" );
           // Wait result...
           var result = Thread.readMessage(true);
           Sys.println(result);
           if (result=="bye")
           {
              going = false;
              try { fromApp.close(); } catch (e:Dynamic) { }
              try { toApp.close(); } catch (e:Dynamic) { }
           }
        }
     }
     trace("Bye bye");
  }



  public static function main()
  {
      var args = Sys.args();
      /*
      for(arg in args)
         if (arg=="-break")
            pause = true;
      */
      var server = new DebugTool(Host.localhost(),8080);

      if (server.ok())
         server.mainLoop();
  }
}

