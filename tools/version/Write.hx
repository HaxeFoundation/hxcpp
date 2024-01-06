import sys.io.File;

class Write
{
   public static function main()
   {
      var args = Sys.args();
      var buildNumber = Std.parseInt(args[0]);
      if (buildNumber<1 || buildNumber==null)
         throw "Usage: Write buildNumber";


      var jsonFile = "haxelib.json";
      var lines = File.getContent(jsonFile).split("\n");
      var idx = 0;
      var versionMatch = ~/(.*"version"\s*:\s*")(.*)(".*)/;
      var found = false;
      var newVersion = "";
      while(idx<lines.length)
      {
         if (versionMatch.match(lines[idx]))
         {
            var parts = versionMatch.matched(2).split(".");
            if (parts.length==3)
               parts[2] = buildNumber+"";
            else
               parts.push(buildNumber+"");
            newVersion = parts.join(".");
            lines[idx]=versionMatch.matched(1) + newVersion + versionMatch.matched(3);
            found = true;
            break;
         }
         idx++;
      }
      if (!found)
         throw "Could not find version in " + jsonFile;

      File.saveContent(jsonFile, lines.join("\n") );

      var writeVersionFilename = "include/HxcppVersion.h";
      var define = "HXCPP_VERSION";
      var lines = [
         '#ifndef $define',
         '#define $define "$newVersion"',
         '#endif'
      ];
      File.saveContent( writeVersionFilename, lines.join("\n") );

      Sys.println("hxcpp_release=" + newVersion );
   }
}
