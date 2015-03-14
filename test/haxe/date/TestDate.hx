package date;
class TestDate extends haxe.unit.TestCase {

	public function testDateAfter2038():Void {
        assertEquals(Date.fromTime(1426355529000).getFullYear(), 2015);
        assertEquals(Date.fromTime(1426355529000*2).getFullYear(), 2060);
	}
}
