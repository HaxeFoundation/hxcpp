#if neko
import neko.net.Socket;
import neko.net.SocketInput;
import neko.net.SocketOutput;
import neko.net.Host;
import neko.vm.Thread;
import neko.Lib;
#else
import cpp.net.Socket;
import cpp.net.Host;
import cpp.Lib;
#end
import haxe.Timer;


class DebugTool
{
   var dbg:Socket;
   var server:Socket;
   var readBuffer:haxe.io.Bytes;
   var fromApp:SocketInput;
   var toApp:SocketOutput;
   var port:Int;
   var mainThread:Thread;
   var socketThread:Thread;
   var keyboardThread:Thread;
   var going:Bool;

   static var BREAK = "X".charCodeAt(0);
   static var CONT  = "c".charCodeAt(0);

  function new(inHost:String, inPort:Int)
  {
     try
     {
        server = new Socket();
        var host = new Host(inHost);
        Lib.print("Waiting for connection on " + host + ":" + inPort + "\n");
        server.bind(host,inPort);
        server.listen(1);
        port = inPort;
        //var connections = Socket.select([server],null,null,null).read;
        //trace("GOT : " + connections);
        //if (connections.length>0)
        {
           dbg = server.accept();
        }
        trace("Got :" + ok());
        if (ok())
        {
           fromApp = dbg.input;
           toApp = dbg.output;
        }
     }
     catch(e:Dynamic)
     {
        Lib.print("Could not create dbg server on " + inHost + ":" + inPort + "\n");
     }
  }

  public function ok() { return dbg!=null; }

  function readString()
  {
     var len = fromApp.readInt31();
     return fromApp.readString(len);
  }

  function socketLoop()
  {
     try
     {
        while(going)
        {
           var appCommand = fromApp.readByte();
           mainThread.sendMessage({from:"socket", data:appCommand});
           var ack = Thread.readMessage(true);
        }
     }
     catch (e:Dynamic)
     {
        mainThread.sendMessage({from:"error", data:"disconnect"});
     }
  }

  function keyboardLoop()
  {
     var stdin = neko.io.File.stdin();
     while(going)
     {
        try
        {
           Lib.print("> ");
           var command = stdin.readLine();
           mainThread.sendMessage({from:"key", data:command});
        }
        catch (e:Dynamic)
        {
           mainThread.sendMessage({from:"error", data:"quit"});
        }
     }
  }

  function processKeyboard(inCommand:String)
  {
     if (inCommand=="break")
     {
        toApp.writeByte(BREAK);
     }
     else if (inCommand=="cont")
     {
        toApp.writeByte(CONT);
     }
  }


  function mainLoop()
  {
     going = true;

     var command = CONT;
     toApp.writeByte(command);

     mainThread = Thread.current();
     keyboardThread = Thread.create(keyboardLoop);
     socketThread = Thread.create(socketLoop);

     while(true)
     {
        var command = Thread.readMessage(true);
        trace(command);
        switch(command.from)
        {
           case "error":
              break;
           case "key":
              processKeyboard(command.data);
           case "socket":
              trace("socket wake!");
        }
     }
     trace("Bye bye");
     going = false;
  }



  public static function main()
  {
      var server = new DebugTool(Host.localhost(),8080);
      if (server.ok())
         server.mainLoop();
  }
}
