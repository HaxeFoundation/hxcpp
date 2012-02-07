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
   var fromApp:haxe.io.Input;
   var toApp:haxe.io.Output;
   var port:Int;
   var mainThread:Thread;
   var socketThread:Thread;
   var keyboardThread:Thread;
   var going:Bool;

   static var BREAK = "X".charCodeAt(0);
   static var CONT  = "c".charCodeAt(0);
   static var WHERE  = "w".charCodeAt(0);
   static var KILL  = "k".charCodeAt(0);
   static var TRACE  = "t".charCodeAt(0);
   static var NOTRACE  = "n".charCodeAt(0);

   static var RESPONSE_RESULT = "s".charCodeAt(0);
   static var RESPONSE_DATA = "d".charCodeAt(0);
   static var RESPONSE_INFO = "i".charCodeAt(0);

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
           var result = "";
           while(true)
           {
              var appResponse = fromApp.readByte();
              if (appResponse==RESPONSE_DATA)
              {
                 result += readString();
              }
              else if (appResponse==RESPONSE_RESULT)
              {
                 result += readString();
                 break;
              }
              else if (appResponse==RESPONSE_INFO)
              {
                 var info = readString();
                 mainThread.sendMessage({from:"info", data:info});
              }
              else
              {
                 result += "Unknown response ??";
                 break;
              }
           }
          
           mainThread.sendMessage({from:"socket", data:result});
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
           var ready = Thread.readMessage(true);

           Lib.print("> ");
           var command = stdin.readLine();
           if (command=="quit")
           {
              mainThread.sendMessage({from:"quit", data:command});
              going = false;
           }
           else
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
     switch(inCommand)
     {
        case "break","b":
           toApp.writeByte(BREAK);

        case "cont","c", "continue":
           toApp.writeByte(CONT);

        case "where","w":
           toApp.writeByte(WHERE);

        case "kill":
           toApp.writeByte(KILL);

        case "trace":
           toApp.writeByte(TRACE);

        case "notrace":
           toApp.writeByte(NOTRACE);

        default:
           Lib.print("Unknown commnd : " + inCommand + ".\n");
           return false;
     }

     return true;
  }


  function mainLoop()
  {
     going = true;

     var command = CONT;
     toApp.writeByte(command);
     var waitingForResponse = true;

     mainThread = Thread.current();
     keyboardThread = Thread.create(keyboardLoop);
     socketThread = Thread.create(socketLoop);

     while(true)
     {
        var command = Thread.readMessage(true);
        //trace(command);
        switch(command.from)
        {
           case "error":
              break;
           case "quit":
              break;
           case "key":
              waitingForResponse = processKeyboard(command.data);
              if (!waitingForResponse)
                 keyboardThread.sendMessage(null);
           case "info":
              Lib.println(command.data);
           case "socket":
              Lib.println(command.data);
              if (waitingForResponse)
              {
                 waitingForResponse = false;
                 keyboardThread.sendMessage(null);
              }
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
