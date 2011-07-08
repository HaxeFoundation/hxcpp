import cpp.zip.Compress;

//XML<xml value="Hello World!"/>

class Test
{
   public static function main()
   {
      #if nme_install_tool
      var bytes:haxe.io.Bytes = ApplicationMain.getAsset("Test.hx");
      #else
      var bytes = cpp.io.File.getBytes("Test.hx");
      #end

      var compressed = cpp.zip.Compress.run(bytes,5);
      var decompressed = cpp.zip.Uncompress.run(compressed);

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
        }
      } catch (e:Dynamic) { }
   }
}
