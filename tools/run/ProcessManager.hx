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
   private static function formatMessage(command:String, args:Array<String>, colorize:Bool = true):String
   {
      var message = "";
      
      if (colorize)
      {
         message = "\x1b[33;1m" + command + "\x1b[0m";
      }
      else
      {
         message = command;
      }
      
      for (arg in args)
      {
         if (colorize)
         {
            var ext = Path.extension(arg);
            if (ext == "cpp" || ext == "c" || ext == "h" || ext == "hpp" || ext == "m" || ext == "mm")
            {
               var split = arg.split ("/");
               if (split.length > 1)
               {
                  arg = "\x1b[33m" + split.slice(0, split.length - 1).join("/") + "/\x1b[33;1m" + split[split.length - 1] + "\x1b[0m";
               }
               else
               {
                  arg = "\x1b[1m" + arg + "\x1b[0m";
               }
            }
            else if (StringTools.startsWith(arg, "-D"))
            {
               arg = "\x1b[1m" + arg + "\x1b[0m";
            }
            else
            {
               arg = "\x1b[0m" + arg + "\x1b[0m";
            }
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
      return message;
   }

   public static function runCommand(path:String, command:String, args:Array<String>, print:Bool = true, safeExecute:Bool = true, ignoreErrors:Bool = false):Int
   {
      if (print && !Log.verbose)
      {
         Log.info(formatMessage(command, args));
      }
      
      command = PathManager.escape(command);
      
      if (safeExecute)
      {
         try
         {
            if (path != null && path != "" && !FileSystem.exists(FileSystem.fullPath(path)) && !FileSystem.exists(FileSystem.fullPath(new Path(path).dir)))
            {
               Log.error("The specified target path \"" + path + "\" does not exist");
               return 1;
            }
            return _runCommand(path, command, args);
         }
         catch (e:Dynamic)
         {
            if (!ignoreErrors)
            {
               var text = formatMessage(command, args);
               Log.error("error running " + text , e);
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
      if (print && !Log.verbose)
      {
         Log.info(formatMessage(command, args));
      }
      
      command = PathManager.escape(command);
      
      if (safeExecute)
      {
         try
         {
            if (path != null && path != "" && !FileSystem.exists(FileSystem.fullPath(path)) && !FileSystem.exists(FileSystem.fullPath(new Path(path).dir)))
            {
               Log.error("The specified target path \"" + path + "\" does not exist");
            }
            return _runProcess(path, command, args, waitForOutput, ignoreErrors);
         }
         catch (e:Dynamic)
         {
            if (!ignoreErrors)
            {
               Log.error("error running " + formatMessage(command,args), e);
            }
            return null;
         }
      }
      else
      {  
         return _runProcess(path, command, args, waitForOutput, ignoreErrors);   
      }
   }
   public static function runProcessLine(path:String, command:String, args:Array<String>, waitForOutput:Bool = true, print:Bool = true, safeExecute:Bool = true, ignoreErrors:Bool = false):String
   {
      var result = runProcess(path, command, args, waitForOutput, print, safeExecute, ignoreErrors);
      if (result!=null)
         return result.split("\n")[0];
      return result;
   }
   
  
   private static function _runCommand(path:String, command:String, args:Array<String>):Int
   {
      var oldPath:String = "";
      
      if (path != null && path != "")
      {  
         Log.info("", " - \x1b[1mChanging directory:\x1b[0m " + path + "");
         
         oldPath = Sys.getCwd();
         Sys.setCwd(path);
      }
      
      Log.info("", " - \x1b[1mRunning command:\x1b[0m " + formatMessage(command, args));
      
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
         Log.info("", " - \x1b[1m - Changing directory:\x1b[0m " + path + "");
         
         oldPath = Sys.getCwd();
         Sys.setCwd(path);
      }
      
      Log.info("", " - \x1b[1mRunning process:\x1b[0m " + formatMessage(command, args));
      
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
                  if (error==null || error=="")
                     error = "error running " + formatMessage(command, args);
                  Log.error(error);
               }

               return null;
            }
         //}
      }
      
      if (oldPath != "")
      {  
         Sys.setCwd(oldPath); 
      }
      
      return output;
   }

   // This function will return 0 on success, or non-zero error code
   public static function runProcessThreaded(command:String, args:Array<String>):Int
   {
      if (!Log.verbose)
         Log.info(formatMessage(command, args));
      
      command = PathManager.escape(command);

      Log.info("", " - \x1b[1mRunning process:\x1b[0m " + formatMessage(command, args));
      
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
      
      if (code != 0)
      {
         if (BuildTool.threadExitCode == 0)
         {
            Log.lock();
            var message = "";
            if (Log.verbose)
            {
               message += '${Log.RED}${Log.BOLD}Error in building thread${Log.NORMAL}\n';
               message += '${Log.ITALIC}' + formatMessage(command,args) + '${Log.NORMAL}\n';
            }
            if (output.length > 0)
            {
               message += output.join("\n") + "\n";
            }
            if (errOut != null)
            {
               message += '${Log.RED}${Log.BOLD}Error:${Log.NORMAL} ${Log.BOLD}' + errOut.join("\n") + '${Log.NORMAL}';
            }
            Log.info (message);
            Log.unlock();
         }
         
         return code;
      }
      
      if (errOut!=null && errOut.length>0)
         output = output.concat(errOut);
         
      if (output.length>0)
      {
         Log.info(output.join("\n"));
      }
      
      return 0;
   }
}
