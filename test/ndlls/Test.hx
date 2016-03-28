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
import sys.io.File;
import sys.io.FileSeek;

using cpp.NativeArray;


class Test
{
   static var errors = new Array<String>();

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
      v("ok");
      return 0;
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
      cnx.request("
        CREATE TABLE IF NOT EXISTS User (
            id INTEGER PRIMARY KEY AUTOINCREMENT, 
            name TEXT, 
            age INTEGER, 
            money DOUBLE
        )
");
      cnx.request("INSERT INTO User (name,age,money) VALUES ('John',32,100.45)");
      cnx.request("INSERT INTO User (name,age,money) VALUES ('Bob',14,4.50)");

      var rset = cnx.request("SELECT * FROM User");

      var length = rset.length;
      v("found "+length+" users");
      if (length!=2)
         return error("Bad user count");
      for( row in rset )
         v("  user "+row.name+" is "+row.age+" years old ");
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
      var dbFile = "mybase.db";
      if (FileSystem.exists(dbFile))
         FileSystem.deleteFile(dbFile);
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
            user : "root",
            pass : "",
            socket : null,
            database : "MyBase"
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

   public static function main()
   {
      var exitCode = 0;

      exitCode |= testDate();
      exitCode |= testCompress();
      exitCode |= testRegexp();
      exitCode |= testSqlite();
      //exitCode |= testMysql();
      exitCode |= testRandom();
      exitCode |= testFile();
      exitCode |= testFileSystem();

      if (exitCode!=0)
         Sys.println("############# Errors running tests:\n   " + errors.join("\n   ") );
      else
         Sys.println("All tests passed.");
      Sys.exit(exitCode);
   }
}
