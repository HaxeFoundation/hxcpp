import haxe.io.BytesOutput;
import haxe.io.Eof;
import haxe.io.Path;
import sys.io.Process;
import sys.FileSystem;

class ProcessManager
{  
   public static function runCommand(path:String, command:String, args:Array<String>, safeExecute:Bool = true, ignoreErrors:Bool = false, print:Bool = false):Int
   {
      if (print)
      {
         var message = command;
         for (arg in args)
         {
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

   public static function runProcess(path:String, command:String, args:Array<String>, waitForOutput:Bool = true, safeExecute:Bool = true, ignoreErrors:Bool = false, print:Bool = false):String
   {
      if (print)
      {
         var message = command;
         for (arg in args)
         {
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
   
   private static function _runCommand(path:String, command:String, args:Array<String>):Int
   {
      var oldPath:String = "";
      
      if (path != null && path != "")
      {  
         LogManager.info("", " - \x1b[1mChanging directory:\x1b[0m " + path + "");
         
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
      
      LogManager.info("", " - \x1b[1mRunning command:\x1b[0m " + command + argString);
      
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
         LogManager.info("", " - \x1b[1mChanging directory:\x1b[0m " + path + "");
         
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
      
      LogManager.info("", " - \x1b[1mRunning process:\x1b[0m " + command + argString);
      
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
}