import haxe.PosInfos;

class TestBase extends haxe.unit.TestCase
{
    public function assertClose(inWant:Float, inGot:Float,  ?c : PosInfos )
    {
       assertTrue( Math.abs(inWant-inGot) < 0.001, c );
    }

    public function assertEq<T>(inWant:T, inGot:T,  ?c : PosInfos )
    {
       var same = inWant==inGot;
       if (!same)
          trace('Expected $inWant, got $inGot');
       assertTrue(same, c);
    }
}

