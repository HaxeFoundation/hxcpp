import haxe.zip.Compress;
import haxe.zip.Uncompress;
import haxe.zip.FlushMode;

//XML<xml value="Hello World!"/>

class Test
{
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
   }
}
