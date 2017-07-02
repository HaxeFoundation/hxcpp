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

      var split = mExe.split(" ");
      var exe = split.shift();
      args = split.concat(args);

      var result = ProcessManager.runCommand("", exe, args);
      if (result!=0)
      {
         Tools.exit(result);
         //throw "Error : " + result + " - build cancelled";
      }
   }
}
