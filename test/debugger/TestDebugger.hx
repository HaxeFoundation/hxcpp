import cpp.vm.Debugger;
#if haxe4
import sys.thread.Thread;
#else
import cpp.vm.Thread;
#end

typedef DebuggerContext = { className : String,
                            functionName : String,
                            fileName : String,
                            lineNumber : Int };

typedef DebuggerTest = { setup:Void->Void,
                         test:DebuggerContext->Bool,
                         name:String,
                         resume:Int->Void };

class TestDebugger
{
   public static var ok = true;
   public static var finished = false;
   static var jobs:Array<DebuggerTest>;
   static var currentTest:DebuggerContext->Bool;
   static var currentName:String;
   static var currentResume:Void->Void;


   public static function setup()
   {
      Debugger.enableCurrentThreadDebugging(false);
      var mainThread = Thread.current();
      Thread.create( function() {
         startDebugger();
         mainThread.sendMessage("setup");
      });

      var message = Thread.readMessage(true);
      Sys.println("Debugger : " + message);
      Debugger.enableCurrentThreadDebugging(true);

   }

   static function handleThreadEvent(threadNumber : Int, event : Int,
                                       stackFrame : Int,
                                       className : String,
                                       functionName : String,
                                       fileName : String, lineNumber : Int)
   {
      if (event==Debugger.THREAD_STOPPED)
      {
         var ctx = { className:className, functionName:functionName, fileName:fileName, lineNumber:lineNumber };

         if (!currentTest(ctx))
         {
            ok = false;
            Sys.println('Test failed : $currentName - got $ctx');
            Sys.exit(-1);
         }
         else
         {
            nextTest(threadNumber);
         }
      }
   }

   static function cont(id:Int)
   {
      Debugger.continueThreads(-1,1);
   }

   static function step(id:Int)
   {
      Debugger.stepThread(id,Debugger.STEP_INTO,1);
   }


   static function stepOver(id:Int)
   {
      Debugger.stepThread(id,Debugger.STEP_OVER,1);
   }


   static function stepOut(id:Int)
   {
      Debugger.stepThread(id,Debugger.STEP_OUT,1);
   }

   static function nextTest(threadId:Int)
   {
      var test = jobs.shift();
      if (test==null)
      {
         finished = true;
         currentName = null;
         currentTest = null;
         currentResume = null;
         Debugger.setEventNotificationHandler(null);
         cont(-1);
      }
      else
      {
         currentName = test.name;
         currentTest = test.test;
         test.setup();
         Sys.println(' $currentName...');
         test.resume(threadId);
      }
   }


   static function startDebugger()
   {
       Debugger.setEventNotificationHandler(handleThreadEvent);
       jobs  = [
          { setup:function()  Debugger.addClassFunctionBreakpoint("App","breakMe"),
            test:function(ctx) return !App.hasRunBreakMe,
            name:"Set function breakpoint App.breakMe",
            resume:cont },
          { setup:function()  Debugger.addClassFunctionBreakpoint("App","breakMe2"),
            test:function(ctx) return App.hasRunBreakMe && !App.hasRunBreakMe2,
            name:"Set function breakpoint App.breakMe2",
            resume:cont},
          { setup:function()  Debugger.addFileLineBreakpoint("Lines.hx",7),
            test:function(ctx) return Lines.line==-1,
            name:"Set line breakpoint Lines.hx:7",
            resume:cont},
          { setup:function()  { },
            test:function(ctx) return Lines.line==7,
            name:"Step from Lines.hx:7",
            resume:step},
          { setup:function()  { },
            test:function(ctx) return Lines.line==18,
            name:"Step over callFunction",
            resume:stepOver},
          { setup:function()  { },
            test:function(ctx) return Lines.line==9,
            name:"Step over line 9",
            resume:step},
          { setup:function()  { },
            test:function(ctx) return Lines.line==9,
            name:"step into callFunction",
            resume:step},
          { setup:function()  { },
            test:function(ctx) return Lines.line==18,
            name:"step out of callFunction",
            resume:stepOut },
          { setup:function()  { },
            test:function(ctx) return Lines.line==12,
            name:"step out of Lines",
            resume:stepOut },


       ];
       nextTest(-1);
   }
}
