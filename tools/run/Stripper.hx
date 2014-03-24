class Stripper
{
   public var mExe:String;
   public var mFlags:Array<String>;

   public function new(inExe:String)
   {
      mFlags = [];
      mExe = inExe;
   }

   public function strip(inTarget:String)
   {
      var args = new Array<String>();
      args = args.concat(mFlags);
      args.push(inTarget);

      var result = BuildTool.runCommand( mExe, args, true,false );
      if (result!=0)
         throw "Error : " + result + " - build cancelled";
   }
}