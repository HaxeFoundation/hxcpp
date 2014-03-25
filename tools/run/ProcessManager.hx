import haxe.io.BytesOutput;
import haxe.io.Eof;
import haxe.io.Path;
import sys.io.Process;
import sys.FileSystem;
#if neko
import neko.vm.Thread;
#else
import cpp.vm.Thread;
#end

class ProcessManager
{
   private static function printCommand(command:String, args:Array<String>):Void
   {
      var message = "\x1b[34;1m" + command + "\x1b[0m";
      for (arg in args)
      {
         if (StringTools.endsWith(arg, ".cpp") || StringTools.endsWith(arg, ".h"))
         {
            arg = "\x1b[1m" + arg + "\x1b[0m";
         }
         
         if (arg.indexOf(" ") > -1)
         {
            message += " \"" + arg + "\"";
         }
         else
         {
            message += " " + arg;
         }
      }
      LogManager.info(message);
   }

   public static function runCommand(path:String, command:String, args:Array<String>, print:Bool = true, safeExecute:Bool = true, ignoreErrors:Bool = false):Int
   {
      if (print && !LogManager.verbose)
      {
         printCommand(command, args);
      }
      
      command = PathManager.escape(command);
      
      if (safeExecute)
      {
         try
         {
            if (path != null && path != "" && !FileSystem.exists(FileSystem.fullPath(path)) && !FileSystem.exists(FileSystem.fullPath(new Path(path).dir)))
            {
               LogManager.error ("The specified target path \"" + path + "\" does not exist");
               return 1;
            }
            return _runCommand(path, command, args);
         }
         catch (e:Dynamic)
         {
            if (!ignoreErrors)
            {
               LogManager.error("", e);
               return 1;
            }
            return 0;
         }
      }
      else
      {
         return _runCommand(path, command, args);
      }
   }

   public static function runProcess(path:String, command:String, args:Array<String>, waitForOutput:Bool = true, print:Bool = true, safeExecute:Bool = true, ignoreErrors:Bool = false):String
   {
      if (print && !LogManager.verbose)
      {
         printCommand(command, args);
      }
      
      command = PathManager.escape(command);
      
      if (safeExecute)
      {
         try
         {
            if (path != null && path != "" && !FileSystem.exists(FileSystem.fullPath(path)) && !FileSystem.exists(FileSystem.fullPath(new Path(path).dir)))
            {
               LogManager.error("The specified target path \"" + path + "\" does not exist");
            }
            return _runProcess(path, command, args, waitForOutput, ignoreErrors);
         }
         catch (e:Dynamic)
         {
            if (!ignoreErrors)
            {
               LogManager.error("", e);
            }
            return null;
         }
      }
      else
      {  
         return _runProcess(path, command, args, waitForOutput, ignoreErrors);   
      }
   }
   
   public static function runProcessThreaded(path:String, command:String, args:Array<String>, print:Bool = true, safeExecute:Bool = true, ignoreErrors:Bool = false):Int
   {
      if (print && !LogManager.verbose)
      {
         printCommand(command, args);
      }
      
      command = PathManager.escape(command);
      
      if (safeExecute)
      {
         try
         {
            if (path != null && path != "" && !FileSystem.exists(FileSystem.fullPath(path)) && !FileSystem.exists(FileSystem.fullPath(new Path(path).dir)))
            {
               LogManager.error("The specified target path \"" + path + "\" does not exist");
            }
            return _runProcessThreaded(path, command, args, ignoreErrors);
         }
         catch (e:Dynamic)
         {
            if (!ignoreErrors)
            {
               LogManager.error("", e);
            }
            return null;
         }
      }
      else
      {  
         return _runProcessThreaded(path, command, args, ignoreErrors);   
      }
   }
   
   private static function _runCommand(path:String, command:String, args:Array<String>):Int
   {
      var oldPath:String = "";
      
      if (path != null && path != "")
      {  
         LogManager.info("", "\x1b[1mChanging directory:\x1b[0m " + path + "");
         
         oldPath = Sys.getCwd();
         Sys.setCwd(path);
      }
      
      var argString = "";
      
      for (arg in args)
      {  
         if (arg.indexOf(" ") > -1)
         {  
            argString += " \"" + arg + "\"";
         }
         else
         {
            argString += " " + arg;
         }
      }
      
      LogManager.info("", "\x1b[1mRunning command:\x1b[0m " + command + argString);
      
      var result = 0;
      
      if (args != null && args.length > 0)
      {
         result = Sys.command(command, args);
      }
      else
      {
         result = Sys.command(command);
      }
      
      if (oldPath != "")
      {
         Sys.setCwd(oldPath);
      }
      
      if (result != 0)
      {  
         throw ("Error running: " + command + " " + args.join (" ") + " [" + path + "]"); 
      }
      
      return result;
   }

   private static function _runProcess(path:String, command:String, args:Array<String>, waitForOutput:Bool, ignoreErrors:Bool):String
   {
      var oldPath:String = "";
      
      if (path != null && path != "")
      {
         LogManager.info("", "\x1b[1mChanging directory:\x1b[0m " + path + "");
         
         oldPath = Sys.getCwd();
         Sys.setCwd(path);
      }
      
      var argString = "";
      
      for (arg in args)
      {
         if (arg.indexOf(" ") > -1)
         {
            argString += " \"" + arg + "\"";
         }
         else
         {
            argString += " " + arg;
         }
      }
      
      LogManager.info("", "\x1b[1mRunning process:\x1b[0m " + command + argString);
      
      var output = "";
      var result = 0;
      
      var process = new Process(command, args);
      var buffer = new BytesOutput();
      
      if (waitForOutput)
      {
         var waiting = true;
         while (waiting)
         {
            try
            {
               var current = process.stdout.readAll(1024);
               buffer.write(current);
               if (current.length == 0)
               {  
                  waiting = false;
               }
            }
            catch (e:Eof)
            {
               waiting = false;
            }
         }
         
         result = process.exitCode();
         process.close();
         
         //if (result == 0)
         //{   
            output = buffer.getBytes().toString();
            if (output == "")
            {
               var error = process.stderr.readAll().toString();
               if (ignoreErrors)
               {
                  output = error;
               }
               else
               {
                  LogManager.error(error);
               }
               
               return null;
               
               /*if (error != "")
               {
                  LogManager.error(error);
               }*/
            }
         //}
      }
      
      if (oldPath != "")
      {  
         Sys.setCwd(oldPath); 
      }
      
      return output;
   }

   private static function _runProcessThreaded(path:String, command:String, args:Array<String>, ignoreErrors:Bool):Int
   {
      var oldPath:String = "";
      
      if (path != null && path != "")
      {
         LogManager.info("", "\x1b[1mChanging directory:\x1b[0m " + path + "");
         
         oldPath = Sys.getCwd();
         Sys.setCwd(path);
      }
      
      var argString = "";
      
      for (arg in args)
      {
         if (arg.indexOf(" ") > -1)
         {
            argString += " \"" + arg + "\"";
         }
         else
         {
            argString += " " + arg;
         }
      }
      
      LogManager.info("", "\x1b[1mRunning process:\x1b[0m " + command + argString);
      
      var output = new Array<String>();
      var process = new Process(command, args);
      var err = process.stderr;
      var out = process.stdout;
      var reader = BuildTool.helperThread.value;
      
      // Read stderr in separate thread to avoid blocking
      if (reader==null)
      {
         var controller = Thread.current();
         BuildTool.helperThread.value = reader = Thread.create(function()
         {
            while(true)
            {
               var stream = Thread.readMessage(true);
               var output:Array<String> = null;
               try
               {
                  while(true)
                  {
                     var line = stream.readLine();
                     if (output==null)
                        output = [ line ];
                     else
                        output.push(line);
                  }
               }
               catch(e:Dynamic){ }
               controller.sendMessage(output);
            }
         });
      }
      
      // Start-up the error reader
      reader.sendMessage(err);

      try
      {
         while(true)
         {
            var line = out.readLine();
            output.push(line);
         }
      }
      catch(e:Dynamic){ }

      var errOut:Array<String> = Thread.readMessage(true);
      
      var code = process.exitCode();
      process.close();
      
      if (code > 0 && !ignoreErrors)
      {
         LogManager.error(errOut.join ("\n"));
      }
      
      if (errOut!=null && errOut.length>0)
         output = output.concat(errOut);
         
      if (output.length>0)
      {
         if (BuildTool.printMutex!=null)
            BuildTool.printMutex.acquire();
         LogManager.info(output.join("\n"));
         if (BuildTool.printMutex!=null)
            BuildTool.printMutex.release();
      }
      
      return code;
   }
}