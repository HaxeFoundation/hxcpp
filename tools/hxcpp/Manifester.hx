class Manifester
{
   public var mExe:String;
   public var mFlags:Array<String>;
   public var mOutPre:String;
   public var mOutPost:String;

   public function new(inExe:String)
   {
      mFlags = [];
      mExe = inExe;
      mOutPre = "";
      mOutPost = "";
   }

   public function add(binName:String,manifestName:String, isExe:Bool)
   {
      var args = new Array<String>();
      args = args.concat(mFlags);

      //only windows for now
      mOutPost = isExe ? ";1" : ";2";

      var result = ProcessManager.runCommand("", mExe, args.concat([manifestName,mOutPre + binName + mOutPost]) );
      if (result!=0)
      {
         Tools.exit(result);
         //throw "Error : " + result + " - build cancelled";
      }
   }
}

