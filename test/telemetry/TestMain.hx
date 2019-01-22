package;

class TestMain {

  static function main(){
    var r = new haxe.unit.TestRunner();
    r.add(new TestBasic());
    var t0 = haxe.Timer.stamp();
    var success = r.run();
    trace(" Time : " + (haxe.Timer.stamp()-t0)*1000 );
    Sys.exit(success ? 0 : 1);
  }
}
