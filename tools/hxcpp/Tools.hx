class Tools {
  private static var onExit:Array<Int->Void> = [];

  public static function addOnExitHook(fn:Int->Void) {
    onExit.push(fn);
  }

  public static function exit(exitCode:Int) {
    for (hook in onExit) {
      hook(exitCode);
    }
    Sys.exit(exitCode);
  }
}
