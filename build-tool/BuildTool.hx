

class Compiler
{
   var mFlags : Array<String>;
   var mCFlags : Array<String>;
   var mCPPFlags : Array<String>;
   var mExe:String;
   var mOutFlag:String;
   var mObjDir:String;

   public function new()
   {
      mFlags = [];
      mCFlags = [];
      mCPPFlags = [];
      mObjDir = "obj";
      mOutFlag = "-o";
      mExe = "";
   }
}

class BuildTool
{
   var mTargets : Array<String>;
   var mDefines : Hash<String>;
   var mCompilers : Hash<Compiler>;


   public function new(inMakefile:String,inTargets:Array<String>,inDefines:Hash<String>)
   {
      mTargets = inTargets;
      mDefines = inDefines;
      mCompilers = new Hash<Compiler>();
      var make_contents = neko.io.File.getContent(inMakefile);
      var xml_slow = Xml.parse(make_contents);
      var xml = new haxe.xml.Fast(xml_slow.firstElement());

      for(el in xml.elements)
      {
         if (valid(el))
         {
            trace(el.name);
            switch(el.name)
            {
                case "set" : 
                   var name = el.att.name;
                   var value = substitute(el.att.value);
                   mDefines.set(name,value);
                case "compiler" : 
                   var id = el.att.id;
                   mCompilers.set(id,createCompiler(el));
            }
         }
      }
   }

   public function createCompiler(inEl:haxe.xml.Fast) : Compiler
   {
      var c = new Compiler();

      return c;
   }

   public function valid(inEl:haxe.xml.Fast) : Bool
   {
      if (inEl.x.get("if")!=null)
         if (!defined(inEl.x.get("if"))) return false;

      if (inEl.has.unless)
         if (defined(inEl.att.unless)) return false;

      return true;
   }

   public function defined(inString:String) : Bool
   {
      return mDefines.exists(inString);
   }
   
   static var mVarMatch = ~/\$(\(.*\))/g;
   public function substitute(str:String) : String
   {
      while( mVarMatch.match(str) )
      {
         str = mVarMatch.matchedLeft() + mDefines.get( mVarMatch.matched(1) ) + mVarMatch.matchedRight();
      }

      trace(str);
      return str;
   }
   
   
   // Process args and environment.
   static public function main()
   {
      var args = neko.Sys.args();
   
      var targets = new Array<String>();
      var defines = new Hash<String>();
      var makefile:String="";
   
      for(arg in args)
      {
         if (arg.substr(0,2)=="-D")
            defines.set(arg.substr(2),"");
         else if (makefile.length==0)
            makefile = arg;
         else
            targets.push(arg);
      }
   
      if (targets.length==0)
      {
         neko.Lib.println("Usage :  BuildTool makefile.xml -DFLAG1 -DFLAG2 ... target1 target2 ...");
      }
      else
      {
         for(env in neko.Sys.environment().keys())
            defines.set(env, neko.Sys.getEnv(env) );
         new BuildTool(makefile,targets,defines);
      }
   
   }
   
}
