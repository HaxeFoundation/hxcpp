import haxe.zip.Compress;
import haxe.zip.Uncompress;
import haxe.zip.FlushMode;
import sys.FileSystem;
import sys.db.Connection;
import sys.db.Sqlite;
import sys.db.Mysql;
import sys.FileSystem;
import haxe.io.Bytes;
import haxe.io.BytesData;

import cpp.Random;
import cpp.net.Poll;
import sys.io.File;
import sys.io.FileSeek;
import sys.net.Host;
import sys.net.Socket;
import sys.net.UdpSocket;
import sys.net.UdpSocket;
import sys.io.Process;

#if haxe4
import sys.thread.Deque;
import sys.thread.Thread;
#else
import cpp.vm.Deque;
import cpp.vm.Thread;
#end

using cpp.NativeArray;
using cpp.AtomicInt;

// These should be ignored in haxe 3.3...
import cpp.link.StaticStd;
import cpp.link.StaticRegexp;
import cpp.link.StaticZlib;
import cpp.link.StaticMysql;
import cpp.link.StaticSqlite;


@:buildXml('<include name="${HXCPP}/src/hx/libs/ssl/Build.xml"/>')
extern class SslTest
{
   @:extern @:native("_hx_ssl_init")
   extern public static function socket_init():Void;
}

class Test
{
   static var errors = new Array<String>();
   static var lastErrorCount = 0;

   var x:Int;

   public function new()
   {
      x = 1;
   }

   public static function log(t:String)
   {
      Sys.println(t);
   }

   public static function v(t:String)
   {
      Sys.println("  " + t);
   }

   public static function ok()
   {
      if (lastErrorCount==errors.length)
      {
         v("ok");
         return 0;
      }
      else
      {
         lastErrorCount=errors.length;
         v("bad");
         return 1;
      }
   }

   public static function error(e:String)
   {
      Sys.println("Test Failed:" + e);
      errors.push(e);
      return -1;
   }

   public static function thisFile()
   {
      #if nme_install_tool
      var bytes:haxe.io.Bytes = ApplicationMain.getAsset("Test.hx");
      #else
      var bytes = sys.io.File.getBytes("Test.hx");
      #end
      return bytes;
   }

   public static function testDb(cnx:Connection) : Int
   {
      v("connected :" + cnx);
      if (cnx.dbName() == "SQLite") {
        cnx.request("
          CREATE TABLE IF NOT EXISTS UserPwd (
              id INTEGER PRIMARY KEY AUTOINCREMENT,
              name TEXT,
              age INTEGER,
              money DOUBLE,
              password BLOB
          )");
      } else {
        cnx.request("
          CREATE TABLE IF NOT EXISTS UserPwd (
              id INTEGER NOT NULL AUTO_INCREMENT,
              name TEXT,
              age INTEGER,
              money DOUBLE,
              password BLOB,
              PRIMARY KEY(id)
          )");
      }
      var dels = cnx.request("DELETE FROM UserPwd");
      if (dels.nfields != 0)
         return error("Bad DELETE'd result");
      v("deleted " + dels.length + " existing rows");

      cnx.request("INSERT INTO UserPwd (name,age,money,password) VALUES ('John',32,100.45,X'c0ffee')");
      cnx.request("INSERT INTO UserPwd (name,age,money,password) VALUES ('Bob',14,4.50,X'deadbeef01020304')");

      var rset = cnx.request("SELECT * FROM UserPwd");

      var length = rset.length;
      v("found "+length+" users");
      if (length!=2)
         return error("Bad user count");
      for( row in rset )
      {
         var pass:Dynamic = row.password;
         var password = Std.is(pass, haxe.io.BytesData) ? haxe.io.Bytes.ofData(pass) : pass;
         var md5 = haxe.crypto.Md5.make(password).toHex().substr(0,8);
         v("  user "+row.name+" is "+row.age+" years old,  password:" + md5);
         if (md5!="5f80e231" && md5!="8ed0b363")
             return error("Bad binary blob store");
      }
      return 0;
   }

   static function testDate()
   {
      log("Test date");
      var now = Date.now();
      v(Std.string(now));
      var then = Date.fromString("1977-06-11");
      v(Std.string(then));
      if (now.getTime()<=then.getTime())
         return error("Date fromString - time travel");

      var later = DateTools.makeUtc(1996,5,4,17,55,11);
      v(Std.string(later));

      var diff:Float = untyped __global__.__hxcpp_timezone_offset(now.mSeconds);
      v("timezone offet:" + diff);

      return ok();
   }

   public static function testCompress()
   {
      log("Test compress");
      var bytes = thisFile();

      var compress = new Compress(9);
      compress.setFlushMode(FlushMode.FINISH);
      var buffer = haxe.io.Bytes.alloc(bytes.length * 2 + 100);
      var r = compress.execute(bytes,0,buffer,0);
      compress.close();
      var compressed = buffer.sub(0,r.write);
      v("compressed size " + compressed.length );

      v("try closing too many times...");
      var caughtError = false;
      try
      {
         compress.close();
      }
      catch(e:Dynamic)
      {
         v("correctly caught " + e );
         caughtError = true;
      }
      if (!caughtError)
         error("Zlib closed without throwing error");

      var decompressed = Uncompress.run(compressed);
      v("decompressed size:" + decompressed.length + "/" + bytes.length);
      if (decompressed.compare(bytes)!=0)
         return error("Compress/Uncompress mismatch");
      return ok();
   }


   public static function testRegexp()
   {
      log("Test Regexp/BytesInput");

      // This utf8 comment is parsed by the code ...
      //  su©©ess
      var success = false;
      var utf8Match = ~/su©+ess/u;

      // This comment is parsed by the code ...
      //XML<xml value="Hello World!"/>
      var match = ~/^..XML/;
      var input = new haxe.io.BytesInput(thisFile());
      try {
        while(true)
        {
           var line = input.readLine();
           if (match.match(line))
           {
              var xml = Xml.parse(line.substr(5));
              v("found xml:" + xml.firstElement().get("value"));
              if (xml.firstElement().get("value")!="Hello World!")
                 return error("Bad universal greeting");
           }
           if (utf8Match.match(line))
           {
              v("found utf8:" + utf8Match.matched(0));
              success = true;
           }
        }
      } catch (e:Dynamic) { }
      if (!success)
         return error("Could not find success in utf8 code");
      return ok();
   }

   public static function testSqlite()
   {
      log("Test sqlite");
      var dbFile = "hxcpp.db";
      var cnx = Sqlite.open(dbFile);
      if (testDb(cnx)!=0)
         return error("db error");
      cnx.close();
      return ok();
   }


   public static function testMysql()
   {
      log("Test mysql");
      var cnx:sys.db.Connection = null;

      try
      {
         cnx = Mysql.connect({ 
            host : "localhost",
            port : 3306,
            user : "hxcpp",
            pass : "hxcpp",
            socket : null,
            database : "hxcpp"
        });
      }
      catch(e:Dynamic)
      {
         v("mysql got error - ok because no server" );
      }

      if (cnx!=null)
      {
        if (testDb(cnx)!=0)
           error("TestDB failed");
        cnx.close();
      }
      return ok();
   }

   public static function testRandom()
   {
      log("Test Random");
      var rand = new Random();
      var f0 = rand.float();
      var f1 = rand.float();
      v('samples $f0,$f1');
      if (f0==f1)
         return error("Not random enough");
      rand.setSeed(1);
      var i0 = rand.int(256);
      rand.setSeed(2);
      var i1 = rand.int(256);
      v('int samples $i0,$i1');
      if (i0!=91 || i1!=217)
         return error("Non-repeatable random seed, should be 91,217");
      var tries = 0;
      while(rand.int(1000)!=999)
         tries++;
      v('tries to 1000 = $tries');
      if (tries!=749)
         return error("Non-repeatable random iterations");

      return ok();
   }

   public static function tryFunc( func ) : Bool
   {
      try
      {
         func();
         return true;
      }
      catch(e:Dynamic) { }
      return false;
   }

   public static function testFile()
   {
      log("Test File");
      try
      {
      var filename = "testfile.txt";
      tryFunc( function() FileSystem.deleteFile(filename) );
      tryFunc( function() FileSystem.deleteFile(filename+"-copy") );
      tryFunc( function() FileSystem.deleteFile(filename+".bin") );
      tryFunc( function() FileSystem.deleteFile(filename+".out") );

      var contents = "line1\nline2\n";

      v("compare...");
      File.saveContent(filename,contents);
      if ( File.getContent(filename)!=contents )
         return error("getContent mismatch");

      v("copy...");
      File.copy(filename,filename+"-copy");
      var bytes = File.getBytes(filename+"-copy");
      if ( bytes.compare( Bytes.ofString(contents) ) !=0 )
         return error("copy getBytes mismatch");

      File.saveBytes(filename+".bin",bytes);

      v("file in...");
      var fileIn = File.read(filename+".bin");
      if (fileIn.readByte()!=contents.charCodeAt(0))
         return error("File readByte mismatch");
      var buffer = Bytes.alloc(5);
      buffer.set(0,'-'.code);
      buffer.set(4,'+'.code);
      if (fileIn.readBytes(buffer,1,3)!=3)
         return error("Could not read 3 bytes");
      v( "read 3: " + buffer.toString() );
      if ( buffer.toString() != "-ine+" )
         return error("Bad sub-buffer readBytes");
      v("seek...");
      if (fileIn.tell()!=4)
         return error("tell!=4");
      fileIn.seek(4, SeekCur );
      if (fileIn.tell()!=8)
         return error("SeekCur tell!=8");
      fileIn.seek(7, SeekBegin );
      if (fileIn.tell()!=7)
         return error("SeekSet tell!=7");
      var rest = Bytes.alloc( contents.length - fileIn.tell() );
      fileIn.readBytes(rest,0,rest.length);
      if (fileIn.eof())
         return error("File at end, but not eof");
      fileIn.seek( -contents.length, SeekEnd );
      if (fileIn.tell()!=0)
         return error("File seek from end to beginning failed");
      fileIn.close();

      v("write...");
      var fileOut = File.write(filename+".out");
      fileOut.writeByte('W'.code);
      if (fileOut.writeBytes(buffer,1,3)!=3)
         return error("Could not write 3 bytes");
      if (fileOut.tell()!=4)
         return error("Bad tell on file write");
      fileOut.flush();
      if (File.getContent(filename+".out")!="Wine")
         return error("Bad reading after flush");
      fileOut.seek(1,SeekBegin);
      fileOut.writeByte('a'.code);
      fileOut.close();
      var contents =  File.getContent(filename+".out");
      v("have :" + contents);
      if (contents!="Wane")
         return error("Bad readback after seek");

      v("cleanup...");
      FileSystem.deleteFile(filename);
      FileSystem.deleteFile(filename + "-copy");
      FileSystem.deleteFile(filename + ".bin");
      FileSystem.deleteFile(filename + ".out");
      return ok();
      }
      catch(e:Dynamic)
      {
         return error("Unexpected error in testFile: " + e);
      }

   }

   public static function testHost()
   {
      log("Test Host");
      try
      {
      var localhost = Host.localhost();
      v('localhost :$localhost');
      var host = new Host(localhost);
      v('host :$host');
      // var reverse = host.reverse();
      // v('reverse :$reverse');
      return ok();
      }
      catch(e:Dynamic)
      {
         return error("Unexpected error in testHost: " + e);
      }

   }


   public static function testFileSystem()
   {
      try
      {

      log("Test FileSystem");
      tryFunc( function() FileSystem.deleteFile("dir/file.txt") );
      tryFunc( function() FileSystem.deleteFile("dir/txt.file") );
      tryFunc( function() FileSystem.deleteFile("dir/child") );
      tryFunc( function() FileSystem.deleteDirectory("dir") );

      v("create dir");
      if (!tryFunc( function() FileSystem.createDirectory("dir") ) )
         return error("Could not create 'dir'");
      if (!tryFunc( function() FileSystem.createDirectory("dir/child") ) )
         return error("Could not create 'dir/child'");
      File.saveContent("dir/file.txt","hello");
      var stat = FileSystem.stat("dir/file.txt");
      v(Std.string(stat));
      if (stat.size!=5)
         return error("File does not contain 5 bytes");
      if ( Std.string(stat.ctime).length != 19)
         return error("File ctime does not appear to be a date");

      v("exists");
      if (!FileSystem.exists("dir"))
         return error("'dir' should exist");
      if (!FileSystem.exists("dir/file.txt"))
         return error("'/file.txt' should exist");
      var files = FileSystem.readDirectory("dir");
      v("dir contents:" + files);
      if (files.length!=2 || files.indexOf("file.txt")<0 || files.indexOf("child")<0)
         return error("Unexpected dir contents " + (files.indexOf("file.txt") + "," + files.indexOf("child")) );
      if (tryFunc( function() FileSystem.deleteDirectory("dir/junk") ) )
         return error("No error deleting junk directory");
      if (tryFunc( function() FileSystem.deleteFile("dir/junk") ) )
         return error("No error deleting junk file");
      if (tryFunc( function() FileSystem.deleteFile("dir/child") ) )
         return error("No error deleting directory as file");
      if (tryFunc( function() FileSystem.deleteDirectory("dir/file.txt") ) )
         return error("No error deleting file as directory");
      var fullPath = FileSystem.fullPath("dir/child");
      v('fullPath: $fullPath');
      var fullPath = FileSystem.fullPath("dir/file.txt");
      v('fullPath: $fullPath');
      v("isDirectory...");
      if (FileSystem.isDirectory("dir/file.txt"))
         return error("file appears to be a directory");
      if (!FileSystem.isDirectory("dir/child"))
         return error("directory appears to not be a directory");
      if (FileSystem.isDirectory("dir/junk"))
         return error("junk appears to be a directory");
      if (FileSystem.isDirectory("dir/file.txt"))
         return error("file appears to be a directory");

      v("rename...");
      if (tryFunc( function() FileSystem.rename("dir/a", "dir/b")) )
         return error("No error renaming missing file");
      FileSystem.rename("dir/file.txt","dir/txt.file");
      if (!FileSystem.exists("dir/txt.file") || FileSystem.exists("dir/file.txt"))
         return error("Rename seemed to go wrong " + FileSystem.readDirectory("dir"));
      v("cleanup..");
      FileSystem.deleteFile("dir/txt.file");
      FileSystem.deleteDirectory("dir/child");
      FileSystem.deleteDirectory("dir");
      if (FileSystem.readDirectory(".").indexOf("dir")>=0)
         return error("Directory removed, but sill there?:" + FileSystem.readDirectory("."));

      return ok();
      }
      catch(e:Dynamic)
      {
         return error("Unexpected error in testFileSystem: " + e);
      }
   }


   public static function testSys()
   {
      log("Test Sys");
      try
      {
         Sys.putEnv("mykey","123");
         var env = Sys.getEnv("mykey");
         v("got env:" + env);
         if (env!="123")
            return error("Bad environment get");
         v("little sleep...");
         var t0 = Sys.time();
         Sys.sleep(0.1);
         var t1 = Sys.time();
         v("Slept for: " + (t1-t0));
         if (t1<=t0 || (t1-t0)>10)
            return error("Too sleepy");
         v("CpuTime: " + Sys.cpuTime());
         v("Cwd: " + Sys.getCwd());
         v("Program Path: " + Sys.programPath());
         var env = Sys.environment();
         v("Environment mykey: " + env.get("mykey") );
         if (env.get("mykey")!="123")
            return error("Could not find mykey in environment");
         v("Ignore getChar auto test");
         v("Args: " + Sys.args());
         v("SystemName: " + Sys.systemName());
         v("Skipping  Sys.setTimeLocale" + Sys.setTimeLocale);
         // Sys.command

         return ok();
      }
      catch(e:Dynamic)
      {
         return error("Unexpected error in testSys: " + e);
      }

   }

   public static function testCommand()
   {
      log("Test Command");
      try
      {
      var code = Sys.command( Sys.programPath(), ["exit", "13"]);
      if (code!=13)
         return error('Process exited with code $code, not 13');

      return ok();
      }
      catch(e:Dynamic)
      {
         return error("Unexpected error in testCommand: " + e);
      }
   }

   public static function runAsProcess()
   {
      var args = Sys.args();
      var job = args.shift();
      if (job=="exit")
      {
         Sys.exit( Std.parseInt(args[0]) );
      }
      else if (job=="socket")
      {
         socketClient();
         Sys.exit(0);
      }
      else
         Sys.println('Unknown job : "$job"');
      Sys.exit(-99);
   }

   public static function testPoll()
   {
      log("Test poll");
      try
      {

      var poll = new Poll(4);
      poll.prepare([],[]);
      var t0 = Sys.time();
      v("poll...");
      poll.poll([],0.1);
      var t = Sys.time()-t0;
      v('took ${t}s');
      /*
      if (t<0.1)
         return error("Timeout too soon");
      if (t>10)
         return error("Timeout too slow");
      */

      return ok();
      }
      catch(e:Dynamic)
      {
         return error("Unexpected error in testPoll: " + e);
      }

   }


   public static function testUdpSocket()
   {
      log("Test UdpSocket");
      try
      {

      var udp = new UdpSocket();
      udp.close();

      return ok();
      }
      catch(e:Dynamic)
      {
         return error("Unexpected error in testUdpSocket: " + e);
      }
   }

   static var socketClientRunning = true;
   public static function readOutput(proc:Process)
   {
      Thread.create( function() {
         while(true)
         {
            try
            {
               var line = proc.stdout.readLine();
               v("---> " + line);
            }
            catch(e:Dynamic)
            {
               break;
            }
         }
         var code = proc.exitCode();
         v("process exit code :" + code);
         socketClientRunning = false;
      });
   }

   public static function testSocket()
   {
      log("Test Socket");
      try
      {
      v("Spawn client..");
      var proc = new Process( Sys.programPath(), ["socket"] );
      readOutput(proc);
      v("connect localhost..");
      var host = new Host("localhost");
      var socket = new Socket();
      socket.bind(host,0xcccc);
      v("listen...");
      socket.listen(1);
      v("accept...");
      var connected = socket.accept();
      v("connected!");

      var input = connected.input;
      var output = connected.output;

      v("send...");
      connected.setFastSend(true);
      connected.write("ping");
      var buffer = Bytes.alloc(8);
      if (input.readBytes(buffer,0,8)!=8)
         return error("Could not read from socket");
      var got = buffer.toString();
      v('got $got');
      if (got!="pingpong")
         return error("Bad socket read");

      v("close connection..");
      connected.close();
      v("close original..");
      socket.close();

      for(wait in 0...10)
      {
         if (!socketClientRunning)
            break;
         v("wait for client...");
         Sys.sleep(0.1);
      }
      if (socketClientRunning)
         return error("Socket client did not finish");

      return ok();
      }
      catch(e:Dynamic)
      {
         return error("Unexpected error in Socket: " + e);
      }

   }

   public static function socketClient()
   {
      log("Client proc...");
      var host = new Host("localhost");
      var socket:Socket = null;
      for(attempt in 0...10)
      {
         try
         {
            log("Client connect...");
            socket = new Socket();
            socket.connect(host,0xcccc);
            break;
         }
         catch(e:Dynamic)
         {
            socket.close();
            socket = null;
            log("Client failed :" + e);
         }
         log("sleep...");
         Sys.sleep(0.25);
      }

      if (socket==null)
         Sys.exit(-1);
      v("connected client");

      var input = socket.input;
      var output = socket.output;

      var buffer = Bytes.alloc(4);
      input.readBytes(buffer,0,4);
      v("client got " + buffer);
      output.writeBytes( Bytes.ofString( buffer.toString() + "pong" ), 0, 8); 
      output.flush();

      v("bye");
      socket.shutdown(true,true);
   }

   public static function testSsl() : Int
   {
      log("Test ssl");
      SslTest.socket_init();
      return ok();
   }

   public static function testSerialization() : Int
   {
      log("Test serialization");
      var orig:haxe.Int64 = haxe.Int64.make(0xdeadbeef,0xbeefdead);
      var recon:haxe.Int64 = haxe.Unserializer.run(haxe.Serializer.run(orig));
      if (orig!=recon)
         error('Bad Int64 serialization $orig != $recon');

      return ok();
   }


   // Hide from optimizer
   static function getOne() return 1;

   public static function testThread()
   {
      log("Test thread");
      v("atomics..");

      var a:AtomicInt = getOne();
      var aPtr = cpp.Pointer.addressOf(a);
      if (aPtr.exchangeIf(2,3))
         error("Bad exchageIf " + a);
      if (!aPtr.exchangeIf(1,3))
         error("No exchageIf " + a);
      if (a!=3)
         error("Bad exchageIf value ");
      if (aPtr.atomicInc()!=3)
         error("Bad atomicInc return");
      if (a!=4)
         error("Bad atomicInc value " + a);
      if (aPtr.atomicDec()!=4)
         error("Bad atomicDec return");
      if (a!=3)
         error("Bad atomicDec value " + a);


      a = getOne();
      v('deque a=$a');

      var q = new Deque<Int>();
      q.add(1);
      q.add(2);
      q.add(3);
      var mainThread = Thread.current();
      // +100000
      Thread.create(function() {
         q.pop(true);
         for(i in 0...100000)
            aPtr.atomicInc();
         mainThread.sendMessage(null);
      });
      // +100000
      Thread.create(function() {
         q.pop(true);
         for(i in 0...100000)
            aPtr.atomicInc();
         mainThread.sendMessage(null);
      } );
      // -200000
      Thread.create(function() {
         q.pop(true);
         for(i in 0...200000)
            aPtr.atomicDec();
         mainThread.sendMessage(null);
      } );

      v("wait for reply..");
      for(i in 0...3)
      {
         Thread.readMessage(true);
         v('got $i');
      }

      if (a!=1)
         error('Bad deque count : $a');

      return ok();
   }

   public static function testFloatReads()
   {
      log("Test float bytes");

      var bytes =haxe.io.Bytes.alloc(1+4+8);
      bytes.fill(0,1,46);

      // Test unaligned read/write
      bytes.setFloat(1,1.25);
      bytes.setDouble(5,1.25);

      if (bytes.get(0)!=46)
         error("Bad byte 0");

      if (bytes.getDouble(5)!=bytes.getFloat(1))
         error("Bad byte read/write");

      return ok();
   }

   public dynamic function getX() return x;

   public static function testDynamicMember()
   {
      log("Test dynamic member");
      var t = new Test();
      if (t.getX()!=1)
         error("Bad dynamic member function");
      return ok();
   }


   @:noDebug
   public static function testNoDebug()
   {
      log("Test noDebug");
      // Just testing to see it it compiles...
      if (  null == new haxe.io.BytesBuffer() )
         error("Bad alloc");
      return ok();
   }

   @:noDebug
   public static function testNoDebugNoAlloc()
   {
      log("Test noDebug, no alloc");
      return ok();
   }


   public static function main()
   {
      var exitCode = 0;
      if (Sys.args().length>0)
      {
         runAsProcess();
      }
      else
      {
         exitCode |= testDate();
         exitCode |= testCompress();
         exitCode |= testRegexp();
         exitCode |= testSqlite();
         exitCode |= testMysql();
         exitCode |= testRandom();
         exitCode |= testFile();
         exitCode |= testFileSystem();
         exitCode |= testHost();
         exitCode |= testSys();
         exitCode |= testCommand();
         exitCode |= testPoll();
         exitCode |= testUdpSocket();
         exitCode |= testSocket();
         exitCode |= testThread();
         exitCode |= testSsl();
         exitCode |= testSerialization();
         exitCode |= testFloatReads();
         exitCode |= testDynamicMember();
         exitCode |= testNoDebug();
         exitCode |= testNoDebugNoAlloc();

         if (exitCode!=0)
            Sys.println("############# Errors running tests:\n   " + errors.join("\n   ") );
         else
            Sys.println("All tests passed.");
         Sys.exit(exitCode);
      }
   }
}
