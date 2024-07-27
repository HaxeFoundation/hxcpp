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

import utest.Assert;

@:buildXml('<include name="${HXCPP}/src/hx/libs/ssl/Build.xml"/>')
extern class SslTest
{
   @:native("_hx_ssl_init")
   extern public static function socket_init():Void;
}

class Test extends utest.Test
{
   var x:Int;

   public function new()
   {
      super();
      
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

   public static function thisFile()
   {
      #if nme_install_tool
      var bytes:haxe.io.Bytes = ApplicationMain.getAsset("Test.hx");
      #else
      var bytes = sys.io.File.getBytes("Test.hx");
      #end
      return bytes;
   }

   function implTestDb(cnx:Connection) : Int
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
         cnx.request("SELECT '豪鬼' test");
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
      Assert.equals(0, dels.nfields, "Bad DELETE'd result");
      v("deleted " + dels.length + " existing rows");

      cnx.request("INSERT INTO UserPwd (name,age,money,password) VALUES ('John',32,100.45,X'c0ffee')");
      cnx.request("INSERT INTO UserPwd (name,age,money,password) VALUES ('Bob',14,4.50,X'deadbeef01020304')");

      var rset = cnx.request("SELECT * FROM UserPwd");

      var length = rset.length;
      v("found "+length+" users");
      Assert.equals(2, length, "Bad user count");
      for( row in rset )
      {
         var pass:Dynamic = row.password;
         var password = Std.isOfType(pass, haxe.io.BytesData) ? haxe.io.Bytes.ofData(pass) : pass;
         var md5 = haxe.crypto.Md5.make(password).toHex().substr(0,8);
         v("  user "+row.name+" is "+row.age+" years old,  password:" + md5);
         Assert.isFalse(md5!="5f80e231" && md5!="8ed0b363", "Bad binary blob store");
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

      Assert.isFalse(now.getTime()<=then.getTime(), "Date fromString - time travel");

      var later = DateTools.makeUtc(1996,5,4,17,55,11);
      v(Std.string(later));

      var diff:Float = untyped __global__.__hxcpp_timezone_offset(now.mSeconds);
      v("timezone offet:" + diff);
   }

   function testCompress()
   {
      log("Test compress");
      final bytes = thisFile();

      final compress = new Compress(9);
      compress.setFlushMode(FlushMode.FINISH);
      final buffer = haxe.io.Bytes.alloc(bytes.length * 2 + 100);
      final r = compress.execute(bytes,0,buffer,0);
      compress.close();
      final compressed = buffer.sub(0,r.write);
      v("compressed size " + compressed.length );

      v("try closing too many times...");
      Assert.exception(() -> compress.close(), String, null, "Zlib closed without throwing error");

      var decompressed = Uncompress.run(compressed);
      v("decompressed size:" + decompressed.length + "/" + bytes.length);
      Assert.equals(0, decompressed.compare(bytes), "Compress/Uncompress mismatch");
   }


   function testRegexp()
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
           final line = input.readLine();
           if (match.match(line))
           {
              final xml = Xml.parse(line.substr(5));
              v("found xml:" + xml.firstElement().get("value"));
              Assert.equals("Hello World!", xml.firstElement().get("value"));
           }
           if (utf8Match.match(line))
           {
              v("found utf8:" + utf8Match.matched(0));
              success = true;
           }
        }
      } catch (e:Dynamic) { }

      Assert.isTrue(success, "Could not find success in utf8 code");
   }

   function testRegexpMixedUnicode() {
      var success = true;

      // when matching a utf8 subject string against a utf16 pattern and vice versa
      for (pattern => subject in ["[A-Za-zÀ-ÖØ-öø-ÿ0-9]+" => "a", "[A-Z]+" => "ÀÖA"]) {
         Assert.isTrue(new EReg(pattern, "").match(subject), '"$subject" does not match against ~/$pattern/');
      }
   }

   function testSqlite()
   {
      log("Test sqlite");
      final dbFile = "hxcpp.db";
      final cnx    = Sqlite.open(dbFile);

      Assert.equals(0, implTestDb(cnx));

      cnx.close();
   }


   function testMysql()
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
        Assert.equals(0, implTestDb(cnx));

        cnx.close();
      }
      else
      {
         Assert.pass();
      }
   }

   function testRandom()
   {
      log("Test Random");

      final rand = new Random();
      final f0 = rand.float();
      final f1 = rand.float();

      v('samples $f0,$f1');
      Assert.notEquals(f0, f1, "Not random enough");

      rand.setSeed(1);
      final i0 = rand.int(256);
      rand.setSeed(2);
      final i1 = rand.int(256);

      Assert.equals( 91, i0, "Non-repeatable random seed");
      Assert.equals(217, i1, "Non-repeatable random seed");

      var tries = 0;
      while(rand.int(1000)!=999)
         tries++;

      Assert.equals(749, tries, "Non-repeatable random iterations");
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

   function testFile()
   {
      log("Test File");
      final filename = "testfile.txt";
      tryFunc( function() FileSystem.deleteFile(filename) );
      tryFunc( function() FileSystem.deleteFile(filename+"-copy") );
      tryFunc( function() FileSystem.deleteFile(filename+".bin") );
      tryFunc( function() FileSystem.deleteFile(filename+".out") );

      final contents = "line1\nline2\n";
      final bytes    = Bytes.ofString(contents);

      v("compare...");
      File.saveContent(filename,contents);
      Assert.equals(contents, File.getContent(filename));

      v("copy...");
      File.copy(filename,filename+"-copy");
      Assert.equals(0, File.getBytes(filename+"-copy").compare(bytes), "copy getBytes mismatch");
      
      v("file in...");
      File.saveBytes(filename+".bin",bytes);
      final fileIn = File.read(filename+".bin");
      Assert.equals(contents.charCodeAt(0), fileIn.readByte(), "File readByte mismatch");
      final buffer = Bytes.alloc(5);
      buffer.set(0,'-'.code);
      buffer.set(4,'+'.code);
      Assert.equals(3, fileIn.readBytes(buffer,1,3), "Could not read 3 bytes");
      Assert.equals("-ine+", buffer.toString(), "Bad sub-buffer readBytes");

      v("seek...");
      Assert.equals(4, fileIn.tell());
      fileIn.seek(4, SeekCur);
      Assert.equals(8, fileIn.tell());
      fileIn.seek(7, SeekBegin );
      Assert.equals(7, fileIn.tell());

      final rest = Bytes.alloc( contents.length - fileIn.tell() );
      fileIn.readBytes(rest,0,rest.length);
      Assert.isFalse(fileIn.eof(), "File at end, but not eof");
      fileIn.seek(-contents.length, SeekEnd);
      Assert.equals(0, fileIn.tell(), "File seek from end to beginning failed");
      fileIn.close();

      v("write...");
      final fileOut = File.write(filename+".out");

      fileOut.writeByte('W'.code);
      Assert.equals(3, fileOut.writeBytes(buffer,1,3), "Could not write 3 bytes");
      Assert.equals(4, fileOut.tell(), "Bad tell on file write");

      fileOut.flush();
      Assert.equals("Wine", File.getContent(filename+".out"), "Bad reading after flush");

      fileOut.seek(1,SeekBegin);
      fileOut.writeByte('a'.code);
      fileOut.close();

      final contents =  File.getContent(filename+".out");
      v("have :" + contents);
      Assert.equals("Wane", contents, "Bad readback after seek");

      v("cleanup...");
      FileSystem.deleteFile(filename);
      FileSystem.deleteFile(filename + "-copy");
      FileSystem.deleteFile(filename + ".bin");
      FileSystem.deleteFile(filename + ".out");
   }

   function testLocalhost()
   {
      log("Test Host");
      
      final localhost = Host.localhost();
      
      v('localhost :$localhost');

      Assert.notNull(localhost);
      Assert.notEquals(0, localhost.length);
   }

   function testHost()
   {
      log("Test Host");
      
      v('host :${ new Host("github.com") }');

      Assert.pass();
   }

   function testFileSystem()
   {
      log("Test FileSystem");
      tryFunc( function() FileSystem.deleteFile("dir/file.txt") );
      tryFunc( function() FileSystem.deleteFile("dir/txt.file") );
      tryFunc( function() FileSystem.deleteFile("dir/child") );
      tryFunc( function() FileSystem.deleteDirectory("dir") );

      v("create dir");
      FileSystem.createDirectory("dir");
      FileSystem.createDirectory("dir/child");
      File.saveContent("dir/file.txt","hello");
      
      final stat = FileSystem.stat("dir/file.txt");
      v(Std.string(stat));

      Assert.equals(5, stat.size, "File does not contain 5 bytes");
      Assert.equals(19, Std.string(stat.ctime).length, "File ctime does not appear to be a date");

      v("exists");
      Assert.isTrue(FileSystem.exists("dir"));
      Assert.isTrue(FileSystem.exists("dir/file.txt"));

      final files = FileSystem.readDirectory("dir");
      v("dir contents:" + files);

      Assert.equals(2, files.length);
      Assert.contains("file.txt", files);
      Assert.contains("child", files);

      Assert.raises(() -> FileSystem.deleteDirectory("dir/junk"));
      Assert.raises(() -> FileSystem.deleteFile("dir/junk"));
      Assert.raises(() -> FileSystem.deleteFile("dir/child"));
      Assert.raises(() -> FileSystem.deleteDirectory("dir/file.txt"));

      final fullPath = FileSystem.fullPath("dir/child");
      v('fullPath: $fullPath');
      final fullPath = FileSystem.fullPath("dir/file.txt");
      v('fullPath: $fullPath');

      v("isDirectory...");
      Assert.isFalse(FileSystem.isDirectory("dir/file.txt"), "file appears to be a directory");
      Assert.isTrue(FileSystem.isDirectory("dir/child"), "directory appears to not be a directory");
      Assert.isFalse(FileSystem.isDirectory("dir/junk"), "junk appears to be a directory");
      Assert.isFalse(FileSystem.isDirectory("dir/file.txt"), "file appears to be a directory");

      v("rename...");

      Assert.exception(() -> FileSystem.rename("dir/a", "dir/b"), String, null, "No error renaming missing file");

      FileSystem.rename("dir/file.txt","dir/txt.file");
      
      Assert.isFalse(FileSystem.exists("dir/file.txt"));
      Assert.isTrue(FileSystem.exists("dir/txt.file"));

      v("cleanup..");
      FileSystem.deleteFile("dir/txt.file");
      FileSystem.deleteDirectory("dir/child");
      FileSystem.deleteDirectory("dir");

      // Assert.equals(0, FileSystem.readDirectory(".").indexOf("dir"), "Directory removed, but sill there?");
      if (FileSystem.readDirectory(".").indexOf("dir")>=0) {
         Assert.fail("Directory removed, but sill there?:" + FileSystem.readDirectory("."));
      }
   }

   function testSys()
   {
      log("Test Sys");

      {
         final key   = "myKey";
         final value = "123";

         Sys.putEnv(key, value);

         Assert.equals(value, Sys.getEnv(key));
         Assert.equals(value, Sys.environment().get(key));
      }

      {
         v("little sleep...");

         final t0 = Sys.time();
         Sys.sleep(0.1);
         final t1 = Sys.time();

         v("Slept for: " + (t1-t0));

         Assert.isFalse(t1<=t0 || (t1-t0)>10, "Too sleepy");
      }
      
      v("CpuTime: " + Sys.cpuTime());
      v("Cwd: " + Sys.getCwd());
      v("Program Path: " + Sys.programPath());
      v("Ignore getChar auto test");
      v("Args: " + Sys.args());
      v("SystemName: " + Sys.systemName());
      v("Skipping  Sys.setTimeLocale" + Sys.setTimeLocale);
   }

   function testCommand()
   {
      log("Test Command");

      final expected = 13;
      
      Assert.equals(expected, Sys.command( Sys.programPath(), ["exit", Std.string(expected)]));
   }

   function testPoll()
   {
      log("Test poll");
      
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

      Assert.pass();
   }


   function testUdpSocket()
   {
      log("Test UdpSocket");
      
      var udp = new UdpSocket();

      udp.close();

      Assert.pass();
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

   function testSocket()
   {
      log("Test Socket");
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

      Assert.equals(8, input.readBytes(buffer,0,buffer.length));
      Assert.equals("pingpong", buffer.toString());

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
      Assert.isFalse(socketClientRunning, "Socket client did not finish");
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

   function testSsl()
   {
      log("Test ssl");

      SslTest.socket_init();

      Assert.pass();
   }

   function testSerialization()
   {
      log("Test serialization");

      var orig:haxe.Int64 = haxe.Int64.make(0xdeadbeef,0xbeefdead);
      var recon:haxe.Int64 = haxe.Unserializer.run(haxe.Serializer.run(orig));

      Assert.equals(orig, recon);
   }

   // Hide from optimizer
   static function getOne() return 1;

   function testThread()
   {
      log("Test thread");
      v("atomics..");

      var a:AtomicInt = getOne();
      var aPtr = cpp.Pointer.addressOf(a);
      Assert.isFalse(aPtr.exchangeIf(2,3));
      Assert.isTrue(aPtr.exchangeIf(1,3));
      Assert.equals(3, a);
      Assert.equals(3, aPtr.atomicInc());
      Assert.equals(4, a);
      Assert.equals(4, aPtr.atomicDec());
      Assert.equals(3, a);

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

      Assert.equals(1, a);
   }

   function testFloatReads()
   {
      log("Test float bytes");

      final value = 46;
      final bytes = haxe.io.Bytes.alloc(1+4+8);
      bytes.fill(0,1,value);

      // Test unaligned read/write
      bytes.setFloat(1,1.25);
      bytes.setDouble(5,1.25);

      Assert.equals(value, bytes.get(0), "Bad byte 0");
      Assert.equals(bytes.getDouble(5), bytes.getFloat(1), "Bad byte read/write");
   }

   public dynamic function getX() return x;

   function testDynamicMember()
   {
      log("Test dynamic member");

      Assert.equals(1, getX(), "Bad dynamic member function");
   }


   @:noDebug
   function testNoDebug()
   {
      log("Test noDebug");

      // Just testing to see it it compiles...
      Assert.notNull(new haxe.io.BytesBuffer(), "bad alloc");
   }

   @:noDebug
   function testNoDebugNoAlloc()
   {
      Assert.pass("Test noDebug, no alloc");
   }

   function testIntParsing()
   {
      log("Test int parsing");

      Assert.equals( 1, Std.parseInt('0x1'));
      Assert.equals( 1, Std.parseInt(' 0x1'));
      Assert.equals( 1, Std.parseInt('\t0x1'));
      Assert.equals( 0, Std.parseInt('   0x'));
      Assert.equals( 0, Std.parseInt('   0xyz'));
      Assert.equals(-1, Std.parseInt('-0x1'));
      Assert.equals(-1, Std.parseInt(' -0x1'));
      Assert.equals(-1, Std.parseInt('\t-0x1'));
      Assert.equals( 0, Std.parseInt('   -0x'));
      Assert.equals( 0, Std.parseInt('   -0xyz'));
      Assert.equals( 5, Std.parseInt('  5'));
      Assert.equals( 5, Std.parseInt(' \t\n5'));
   }

   public static function main()
   {
      switch Sys.args() {
         case [ 'exit', code ]:
            Sys.exit(Std.parseInt(code));
         case [ 'socket' ]:
            socketClient();
         case _:
            utest.UTest.run([ new Test() ]);
      }
   }
}
