import haxe.zip.Compress;
import haxe.zip.Uncompress;
import haxe.zip.FlushMode;
import sys.FileSystem;
import sys.db.Connection;
import sys.db.Sqlite;
import sys.db.Mysql;

#if static_ndll
import hxcpp.StaticStd;
import hxcpp.StaticRegexp;
import hxcpp.StaticZlib;
import hxcpp.StaticMysql;
import hxcpp.StaticSqlite;
#end


//XML<xml value="Hello World!"/>

class Test
{
   public static function testDb(cnx:Connection)
   {
      trace(cnx);
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
      Sys.println("Found "+rset.length+" users");
      for( row in rset )
         Sys.println("User "+row.name+" is "+row.age+" years old ");
   }

   public static function main()
   {
      #if nme_install_tool
      var bytes:haxe.io.Bytes = ApplicationMain.getAsset("Test.hx");
      #else
      var bytes = sys.io.File.getBytes("Test.hx");
      #end

      var compress = new Compress(9);
      compress.setFlushMode(FlushMode.FINISH);
      var buffer = haxe.io.Bytes.alloc(bytes.length * 2 + 100);
      var r = compress.execute(bytes,0,buffer,0);
      compress.close();
      var compressed = buffer.sub(0,r.write);

      var caughtError = false;
      try
      {
         compress.close();
      }
      catch(e:Dynamic)
      {
         Sys.println("That was close (" + e + ")");
         caughtError = true;
      }
      if (!caughtError)
         throw("Oops, I closed it again");

      var decompressed = Uncompress.run(compressed);

      //  su©©ess
      var success = false;
      var utf8Match = ~/su©+ess/u;
      var match = ~/^..XML/;
      try {
        var input = new haxe.io.BytesInput(decompressed);
        while(true)
        {
           var line = input.readLine();
           if (match.match(line))
           {
              var xml = Xml.parse(line.substr(5));
              trace(xml.firstElement().get("value"));
           }
           if (utf8Match.match(line))
           {
              trace(utf8Match.matched(0));
              success = true;
           }
        }
      } catch (e:Dynamic) { }
      if (!success)
        throw "Could not find success in utf8 code";

      var dbFile = "mybase.db";
      if (FileSystem.exists(dbFile))
         FileSystem.deleteFile(dbFile);
      var cnx = Sqlite.open(dbFile);
      testDb(cnx);
      cnx.close();

      try
      {
         var cnx = Mysql.connect({ 
            host : "localhost",
            port : 3306,
            user : "root",
            pass : "",
            socket : null,
            database : "MyBase"
        });
        testDb(cnx);
        cnx.close();
      } catch(e:Dynamic) {
         trace("Mysql is untested : " + e );
      }
   }
}
