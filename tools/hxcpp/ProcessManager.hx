import haxe.io.BytesOutput;
import haxe.io.Eof;
import haxe.io.Path;
import sys.io.Process;
import sys.FileSystem;
#if haxe4
import sys.thread.Thread;
#elseif neko
import neko.vm.Thread;
#else
import cpp.vm.Thread;
#end

class ProcessManager
{
   static function dup(inArgs:Array<String>)
   {
      if (inArgs==null)
         return [];
      return inArgs.copy();
   }

   // Command may be a pseudo command, like "xcrun --sdk abc", or 'python "some script"'
   // Here we split the first word into command and move the rest into args, being careful
   //  to preserve quoted words
   static function combineCommand(command:String, args:Array<String>)
   {
      var parts = new Array<String>();
      var c = command;
      var quoted = ~/^\s*"([^"]+)"(.*)/;
      var word = ~/^\s*(\S+)(.*)/;
      while(c.length>0)
      {
         if (quoted.match(c))
         {
            parts.push( quoted.matched(1) );
            c = quoted.matched(2);
         }
         else if (word.match(c))
         {
            parts.push( word.matched(1) );
            c = word.matched(2);
         }
         else
            break;
      }
      if (parts.length>1)
      {
         command = parts.shift();
         while(parts.length>0)
            args.unshift( parts.pop() );
      }
      return PathManager.escape(command);
   }

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

   public static function runCommand(path:String, command:String, args:Array<String>, print:Bool = true, safeExecute:Bool = true, ignoreErrors:Bool = false,?inText:String):Int
   {
      args = dup(args);
      command = combineCommand(command,args);


      if (print && !Log.verbose && !Log.quiet)
      {
         Log.info(inText==null ? "" : inText,formatMessage(command, args));
      }

      if (safeExecute)
      {
         try
         {
            if (path != null && path != "" && !FileSystem.exists(FileSystem.fullPath(path)) && !FileSystem.exists(FileSystem.fullPath(new Path(path).dir)))
            {
               Log.error("The specified target path \"" + path + "\" does not exist");
               return 1;
            }
            return _runCommand(path, command, args, inText);
         }
         catch (e:Dynamic)
         {
            if (!ignoreErrors)
            {
               //var text = formatMessage(command, args);
               //Log.error("Error while running command\n" + text , e);
               if (Log.verbose)
               {
                  Log.error ("", e);
               }
               return 1;
            }
            return 0;
         }
      }
      else
      {
         return _runCommand(path, command, args, inText);
      }
   }

   public static function readStderr(inCommand:String,inArgs:Array<String>)
   {
      inArgs = dup(inArgs);
      inCommand = combineCommand(inCommand,inArgs);

      var result = new Array<String>();
      var proc = new Process(inCommand,inArgs);
      try
      {
         while(true)
         {
            var out = proc.stderr.readLine();
            result.push(out);
         }
      } catch(e:Dynamic){}
      proc.close();
      return result;
   }

   public static function readStdout(command:String,args:Array<String>)
   {
      args = dup(args);
      command = combineCommand(command,args);


      var result = new Array<String>();
      var proc = new Process(command,args);
      try
      {
         while(true)
         {
            var out = proc.stdout.readLine();
            result.push(out);
         }
      } catch(e:Dynamic){}
      proc.close();
      return result;
   }

   public static function runProcess(path:String, command:String, args:Array<String>, waitForOutput:Bool = true, print:Bool = true, safeExecute:Bool = true, ignoreErrors:Bool = false, ?text:String):String
   {
      args = dup(args);
      command = combineCommand(command,args);

      if (print && !Log.verbose)
      {
         Log.info(formatMessage(command, args));
      }


      if (safeExecute)
      {
         try
         {
            if (path != null && path != "" && !FileSystem.exists(FileSystem.fullPath(path)) && !FileSystem.exists(FileSystem.fullPath(new Path(path).dir)))
            {
               Log.error("The specified target path \"" + path + "\" does not exist");
            }
            return _runProcess(path, command, args, waitForOutput, ignoreErrors, text);
         }
         catch (e:Dynamic)
         {
            if (!ignoreErrors)
            {
               //Log.error("Error while running command\n" + formatMessage(command,args), e);
               if (Log.verbose)
               {
                  Log.error ("", e);
               }
            }
            return null;
         }
      }
      else
      {  
         return _runProcess(path, command, args, waitForOutput, ignoreErrors, text);
      }
   }
   public static function runProcessLine(path:String, command:String, args:Array<String>, waitForOutput:Bool = true, print:Bool = true, safeExecute:Bool = true, ignoreErrors:Bool = false):String
   {
      var result = runProcess(path, command, args, waitForOutput, print, safeExecute, ignoreErrors);
      if (result!=null)
         return result.split("\n")[0];
      return result;
   }
   
  
   private static function _runCommand(path:String, command:String, args:Array<String>, inText:String):Int
   {
      var oldPath:String = "";
      
      if (path != null && path != "")
      {  
         Log.info("", " - \x1b[1mChanging directory:\x1b[0m " + path + "");
         
         oldPath = Sys.getCwd();
         Sys.setCwd(path);
      }
      
      if (Log.quiet && inText!=null)
      {
         Log.info(inText);
      }
      else
      {
         var text = inText==null ?  "Running command" : inText;
         Log.info("", " - \x1b[1m" + text + ":\x1b[0m " + formatMessage(command, args));
      }
      
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
         throw ("Error while running command\n" + formatMessage(command, args) + (path != "" ? " [" + path + "]" : "")); 
      }
      
      return result;
   }

   private static function _runProcess(path:String, command:String, args:Array<String>, waitForOutput:Bool, ignoreErrors:Bool, inText:String):String
   {
      var oldPath:String = "";
      
      if (path != null && path != "")
      {
         Log.info("", " - \x1b[1m - Changing directory:\x1b[0m " + path + "");
         
         oldPath = Sys.getCwd();
         Sys.setCwd(path);
      }

      if ( !Log.quiet)
      {
         var text = inText==null ? "Running process" : inText;
         Log.info("", " - \x1b[1m" + text + ":\x1b[0m " + formatMessage(command, args));
      }
      else
      {
         Log.info("",inText);
      }
      
      var output = "";
      var result = 0;
      
      var process:Process = null;
      try
      {
         process = new Process(command, args);
      }
      catch(e:Dynamic)
      {
         if (ignoreErrors)
            return null;
         Log.error(e+"");
      }

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
                     error = "Error while running command\n" + formatMessage(command, args);
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
   public static function runProcessThreaded(command:String, args:Array<String>, inText:String = null):Int
   {
      args = dup(args);
      command = combineCommand(command,args);

      Log.lock();

      // Other thread may have already thrown an error
      if (BuildTool.threadExitCode!=0)
      {
         Log.unlock();
         return BuildTool.threadExitCode;
      }

      if (inText != null)
         Log.info(inText,"");

      if (!Log.quiet)
         Log.v(" - \x1b[1mRunning command:\x1b[0m " + formatMessage(command, args));
      Log.unlock();

      var output = new Array<String>();
      var process:Process = null;
      try
      {
         process = new Process(command, args);
      }
      catch(e:Dynamic)
      {
         Log.lock();
         if (BuildTool.threadExitCode == 0)
         {
            Log.info('${Log.RED}${Log.BOLD}$e${Log.NORMAL}\n');
            BuildTool.setThreadError(-1);
         }
         Log.unlock();
         return -1;
      }


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

      if (output.length==1 && ~/^\S+\.(cpp|c|cc)$/.match(output[0]))
      {
         // Microsoft prints the name of the cpp file for some reason
         output = [];
      }

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
               Log.println("");
               message += "Error while running command\n";
               message += formatMessage(command,args) + "\n\n";
            }
            if (output.length > 0)
            {
               message += output.join("\n") + "\n";
            }
            if (errOut != null)
            {
               message += errOut.join("\n") + '${Log.NORMAL}';
            }
            Log.error(message,"",null,false);
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
