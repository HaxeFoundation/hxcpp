class MkOps
{
   public static function main()
   {
      var file = [];
      file.push("enum E { EVal0; EVal1; }");
      file.push("class Ops {");
      file.push("  public static var data: { ?f:Float };");
      file.push("  public static function check(b:Bool) { }");
      file.push("  public static function main() {");
      file.push("  var d:Dynamic = null;");
      file.push("  var ai = [1]; var fi=[1.2];");
      file.push("  var int:Int=0;");
      file.push("  var anon={a:Int, b:[2], c:EVal0};");
      file.push("  var anon2:Dynamic={a:1};");
      file.push("  var dynArray:Array<Dynamic> = [1];");
      file.push("  var uint8:cpp.UInt8 = 1;");
      file.push("  var int8:cpp.Int8 = 1;");
      file.push("  var uint16:cpp.UInt16 = 1;");
      file.push("  var int16:cpp.Int16 = 1;");
      file.push("  var uint64:cpp.UInt64 = 1;");
      file.push("  var int64:cpp.Int64 = 1;");
      file.push("  var string = 'S0';");
      file.push("  var arrarr = [ [12] ];");
      file.push("  var arrdyn = [ [d] ];");
      file.push("  var arrdynarray = [ [dynArray] ];");
      file.push("  var eval = EVal0;");

      var exprs = [ "d", "null", "int", "ai", "ai[0]", "fi", "fi[0]", "anon.a", "anon.b", "anon.c", "anon", "anon2.xyz", "dynArray", "dynArray[0]", "3.8", '"Hello"', "uint8", "int8", "uint16", "int16", "uint64", "int64", "string", "arrarr", "arrarr[0]", "arrarr[0][0]", "arrdyn", "arrdynarray", "EVal1", "eval", "data.f" ];

      var total = 0;
      for(e1 in exprs)
      {
         for(e2 in exprs)
         {
            file.push('check( $e1 != $e2 );');
            file.push('check( $e1 == $e2 );');
            file.push('check( $e1 > $e2 );');
            file.push('$e1 = $e2;');
            total += 4;
            if (!skipPlus(e1) && !skipPlus(e2))
            {
               file.push('$e1 += $e2;');
               file.push('$e1 -= $e2;');
               file.push('$e1 /= $e2;');
               file.push('$e1 / $e2;');
               file.push('$e1 *= $e2;');
               total +=5;
            }
         }
      }
      file.push("}}");

      // Pass0 catches common stuff
      // Pass1 checks for "null on static targets"
      // Pass2 should work
      var errors = 0;
      for(pass in 0...3)
      {
         sys.io.File.saveContent("Ops.hx", file.join("\n") );

         var proc = new sys.io.Process("haxe", ["-cpp", "cpp", "-D", "no-compilation", "-main", "Ops"] );
         try
         {
            var stderr = proc.stderr;
            var errMatch = ~/^Ops.hx:(\d+):(.*)/;
            while(true)
            {
               var line = stderr.readLine();
               if (errMatch.match(line))
               {
                  var errLine = Std.parseInt(errMatch.matched(1));
                  file[errLine-1] = "// " + errMatch.matched(2) + " " + file[errLine-1];
                  errors++;
               }
            }
         }
         catch(e:Dynamic) { }
         var code = proc.exitCode();
         Sys.println(' pass error $code, total errors = $errors/$total');
         if (pass==2 && code!=0)
         {
            Sys.println("Still errors after 3 passes - aborting");
            Sys.exit(-1);
         }
      }

      var code = Sys.command("haxe", ["-cpp", "cpp", "-main", "Ops"] );
      Sys.println("Combos  : " + total);
      Sys.println("Exit code : " + code);
      Sys.exit(code);
   }

   // + and arrays do not mix...
   static function skipPlus(e:String)
   {
      return e=="ai" || e=="arrarr" || e=="arrdyn" || e=="arrarr[0]" || e=="fi" ||
         e=="arrdynarray" || e=="dynArray" || e=="eval" || e=="anon.b" || e== "EVal1";
   }
}
