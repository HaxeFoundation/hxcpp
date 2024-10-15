import haxe.Exception;
import haxe.Json;
import sys.io.File;

using StringTools;

typedef Haxelib = {
   var version: String;
}

class Write
{
   public static function main()
   {
      switch Sys.args()
      {
         case [ version ] if (version.startsWith('v')):
            switch version.substr(1).split('.')
            {
               case [ previousMajor, previousMinor, previousPatch ]:
                  final jsonFile = "haxelib.json";
                  final json     = (cast Json.parse(File.getContent(jsonFile)) : Haxelib);

                  switch json.version.split('.')
                  {
                     case [ newMajor, newMinor, _ ]:
                        if (newMajor < previousMajor || (newMajor == previousMajor && newMinor < previousMinor))
                        {
                           throw new Exception('Version in haxelib.json is older than the last tag');
                        }

                        if (newMajor > previousMajor || newMinor > previousMinor)
                        {
                           json.version = '$newMajor.$newMinor.0';
                        }
                        else
                        {
                           json.version = '$newMajor.$newMinor.${ Std.parseInt(previousPatch) + 1 }';
                        }
                     case _:
                        throw new Exception('Invalid version in haxelib.json');
                  }

                  File.saveContent(jsonFile, Json.stringify(json, '\t'));

                  final define = "HXCPP_VERSION";
                  final lines  = [
                     '#ifndef $define',
                     '#define $define "${ json.version }"',
                     '#endif'
                  ];

                  File.saveContent("include/HxcppVersion.h", lines.join("\n"));

                  Sys.println("hxcpp_release=" + json.version );
               case _:
                  throw new Exception('Invalid version in tag');
            }
         case other:
            throw new Exception('Invalid version $other');
      }
   }
}
