class Common
{
   public static var status:String = "tests not run";
   public static var hostImplementation:pack.HostInterface;
   public static var clientImplementation:pack.HostInterface;
   public static var clientRoot:HostRoot;

   public static var callbackSet:Int = 0;
   public static var callback: Void->Void;

	public static var count = 0;
	public static function incrementCount():Int {
		return count++;
	}

   public function new() {}

   public function dummyMethod() {}
   public function dummyMethodArg(_) {}

	public function instanceIncrementCount(_) {
      count++;
   }
}
