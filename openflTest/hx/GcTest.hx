package hx;
import massive.munit.Assert;
import cpp.vm.Gc;
class GcTest {

	@Test
	public function testObjectLeak():Void {
        var object:CustomObject = new CustomObject();
        Gc.doNotKill(object);
        Gc.run(true);
        Assert.isNull(Gc.getNextZombie());
	}

    @Test
    public function testObjectNonLeak():Void {
        var object:CustomObject = new CustomObject();
        Gc.doNotKill(object);
        object = null;
        Gc.run(true);
        Assert.isNotNull(Gc.getNextZombie());
    }

    @Test
    public function testBitmapDataLeak():Void {
//        var object:BitmapData = new BitmapData(1,1);
//        Gc.doNotKill(object);
//        Gc.run(true);
//        Assert.isNull(Gc.getNextZombie());
    }

}