class Common
{
   public static var status:String = "tests not run";
   public static var hostImplementation:pack.HostInterface;
   public static var clientImplementation:pack.HostInterface;
   public static var clientRoot:HostRoot;

   public static var callbackSet:Int = 0;
   public static var callback: Void->Void;

}
