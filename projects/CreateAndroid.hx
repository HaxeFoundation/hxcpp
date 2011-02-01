class CreateAndroid
{
   public function new(inHXCPP:String,inPackage:String)
   {
      cp_recurse(inHXCPP + "/projects/android/template",".");
   }

   static public function cp_recurse(inSrc:String,inDestDir:String)
   {
      if (!neko.FileSystem.exists(inDestDir))
      {
         neko.Lib.println("mkdir " + inDestDir);
         neko.FileSystem.createDirectory(inDestDir);
      }

      var files = neko.FileSystem.readDirectory(inSrc);
      for(file in files)
      {
         if (file.substr(0,1)!=".")
         {
            var dest = inDestDir + "/" + file;
            var src = inSrc + "/" + file;
            if (neko.FileSystem.isDirectory(src))
               cp_recurse(src, dest );
            else
            {
               neko.Lib.println("cp " + src + " " + dest );
               neko.io.File.copy( src, dest );
            }
         }
      }
   }


   static public function mkdir(inDir:String)
   {
      var parts = inDir.split("/");
      var total = "";
      for(part in parts)
      {
         if (part!="." && part!="")
         {
            if (total!="") total+="/";
            total += part;
            if (!neko.FileSystem.exists(total))
            {
               neko.Lib.println("mkdir " + total);
               neko.FileSystem.createDirectory(total);
            }
         }
      }
   }


   
   public static function main()
   {
      var args = neko.Sys.args();
      if (args.length!=1)
      {
         neko.Lib.println("Usgage: CreateAndroid com.yourcompany.yourapp");
      }
      else
      {
         var proc = new neko.io.Process("haxelib",["path", "hxcpp"]);
         var hxcpp = proc.stdout.readLine();
         proc.close();
         trace(hxcpp);
   
         new CreateAndroid(hxcpp,args[0]);
      }
   }


}



