interface IClientInterface
{
   // Same as IHostInterface, but not implemented in host
   public function whoStartedYou() : String;
   public function whoOverridesYou() : String;
   public function uniqueClientFunc() : String;
}
